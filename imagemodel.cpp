#include "imagemodel.h"
#include <QDir>
#include <QMetaObject>
#include <QFile>
#include <QImageReader>
#include <QtConcurrent/QtConcurrent>
#include <tiffio.h>

// Вспомогательная функция для размера
static QString humanReadableSize(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB"};
    int i = 0;
    while (size >= 1024.0 && i < units.size() - 1) {
        size /= 1024.0;
        ++i;
    }
    return QString("%1 %2").arg(QString::number(size, 'f', 2)).arg(units[i]);
}

ImageModel::ImageModel(QObject *parent)
    : QAbstractTableModel(parent) {}

// ===== Чтение размеров и DPI из PCX ====
static QSize getPCXSize(const QString &path, int &depth, int &dpiX, int &dpiY) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    if (f.size() < 128) return {};
    QByteArray header = f.read(128);
    const uchar *h = reinterpret_cast<const uchar*>(header.constData());

    int xMin = h[4] | (h[5] << 8);
    int yMin = h[6] | (h[7] << 8);
    int xMax = h[8] | (h[9] << 8);
    int yMax = h[10] | (h[11] << 8);

    depth = h[3]; // bits per pixel per plane
    int planes = h[65];
    depth *= planes;

    dpiX = h[12] | (h[13] << 8);
    dpiY = h[14] | (h[15] << 8);

    return QSize(xMax - xMin + 1, yMax - yMin + 1);
}

// ===== Чтение TIFF через libtiff =====
static bool getTIFFInfo(const QString &path, QSize &dimensions, int &depth,
                        int &dpiX, int &dpiY, QString &compression) {
    QByteArray localPath = QDir::toNativeSeparators(path).toLocal8Bit();
    TIFF *tif = TIFFOpen(localPath.constData(), "r");
    if (!tif) return false;

    uint32 width = 0, height = 0;
    uint16 bitsPerSample = 0, samplesPerPixel = 0, comp = 0;
    float xres = 0, yres = 0;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    TIFFGetField(tif, TIFFTAG_COMPRESSION, &comp);
    TIFFGetField(tif, TIFFTAG_XRESOLUTION, &xres);
    TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yres);

    TIFFClose(tif);

    if (width == 0 || height == 0)
        return false;

    dimensions = QSize(width, height);
    depth = bitsPerSample * samplesPerPixel;
    dpiX = (xres > 0) ? int(xres) : 0;
    dpiY = (yres > 0) ? int(yres) : 0;

    switch (comp) {
    case COMPRESSION_NONE: compression = "Uncompressed"; break;
    case COMPRESSION_LZW: compression = "LZW (Lossless)"; break;
    case COMPRESSION_PACKBITS: compression = "PackBits (Lossless)"; break;
    case COMPRESSION_CCITTFAX3: compression = "CCITT Group 3"; break;
    case COMPRESSION_CCITTFAX4: compression = "CCITT Group 4"; break;
    case COMPRESSION_DEFLATE: compression = "Deflate (Lossless)"; break;
    default: compression = QString("TIFF Compression %1").arg(comp); break;
    }

    return true;
}

// ===== Загрузка изображений из папки =====
void ImageModel::loadFromFolder(const QString &folderPath) {
    beginResetModel();
    images.clear();
    endResetModel();

    QDir dir(folderPath);
    QStringList filters = {"*.jpg", "*.jpeg", "*.gif", "*.tif", "*.tiff",
                           "*.bmp", "*.png", "*.pcx"};
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    QStringList paths;
    for (const QFileInfo &fi : fileList)
        paths << fi.absoluteFilePath();

    QtConcurrent::run([this, paths]() {
        const int batchSize = 20;
        QList<ImageInfo> batch;
        batch.reserve(batchSize);

        int processed = 0;
        for (const QString &path : paths) {
            QFileInfo fileInfo(path);
            QImageReader reader(path);

            ImageInfo info;
            info.fileName = fileInfo.fileName();
            info.fileSize = fileInfo.size();
            info.format = QString(reader.format()).toUpper();

            if (info.format.isEmpty()) {
                QString suffix = fileInfo.suffix().toUpper();
                if (suffix == "PCX") info.format = "PCX";
                else if (suffix == "TIF" || suffix == "TIFF") info.format = "TIFF";
                else info.format = "Unknown";
            }

            if (info.format == "PCX") {
                int depth = 0, dpiX = 0, dpiY = 0;
                info.dimensions = getPCXSize(path, depth, dpiX, dpiY);
                info.depth = depth;
                info.dpiX = dpiX;
                info.dpiY = dpiY;
                info.compression = "RLE (Lossless)";
            }
            // TIFF
            else if (info.format == "TIFF") {
                QSize dim;
                int depth = 0, dpiX = 0, dpiY = 0;
                QString comp;
                if (getTIFFInfo(path, dim, depth, dpiX, dpiY, comp)) {
                    info.dimensions = dim;
                    info.depth = depth;
                    info.dpiX = dpiX;
                    info.dpiY = dpiY;
                    info.compression = comp;
                } else {
                    info.dimensions = QSize();
                    info.depth = 0;
                    info.dpiX = info.dpiY = 0;
                    info.compression = "Unreadable TIFF";
                }
            }
            // Остальные форматы
            else {
                if (reader.supportsOption(QImageIOHandler::Size))
                    info.dimensions = reader.size();

                QImage img = reader.read();
                if (!img.isNull()) {
                    info.dpiX = (img.dotsPerMeterX() > 0) ? qRound(img.dotsPerMeterX() * 0.0254) : 0;
                    info.dpiY = (img.dotsPerMeterY() > 0) ? qRound(img.dotsPerMeterY() * 0.0254) : 0;
                    info.depth = img.depth();
                } else {
                    info.dpiX = info.dpiY = 0;
                    info.depth = 0;
                }

                if (info.format == "JPG" || info.format == "JPEG")
                    info.compression = "Lossy";
                else if (info.format == "PNG" || info.format == "GIF")
                    info.compression = "Lossless";
                else if (info.format == "BMP")
                    info.compression = "Uncompressed / RLE (Lossless)";
                else
                    info.compression = "N/A";
            }

            batch.append(info);
            processed++;

            if (batch.size() >= batchSize) {
                QMetaObject::invokeMethod(this, [this, batch]() {
                    beginInsertRows(QModelIndex(), images.size(),
                                    images.size() + batch.size() - 1);
                    images.append(batch);
                    endInsertRows();
                    emit batchLoaded(images.size());
                }, Qt::QueuedConnection);
                batch.clear();
            }
        }

        if (!batch.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, batch]() {
                beginInsertRows(QModelIndex(), images.size(),
                                images.size() + batch.size() - 1);
                images.append(batch);
                endInsertRows();
                emit batchLoaded(images.size());
            }, Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(this, [this, processed]() {
            emit loadingFinished(processed);
        }, Qt::QueuedConnection);
    });
}

int ImageModel::rowCount(const QModelIndex &) const {
    return images.size();
}

int ImageModel::columnCount(const QModelIndex &) const {
    return 7;
}

QVariant ImageModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    const ImageInfo &info = images[index.row()];

    if (role == Qt::ToolTipRole && index.column() == 0) {
        return info.fileName;
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return info.fileName;
        case 1: return humanReadableSize(info.fileSize);
        case 2: return info.dimensions.isValid()
                       ? QString("%1 x %2").arg(info.dimensions.width()).arg(info.dimensions.height())
                       : "N/A";
        case 3: return (info.dpiX > 0 && info.dpiY > 0)
                       ? QString("%1 x %2").arg(info.dpiX).arg(info.dpiY)
                       : "N/A";
        case 4:
            if (info.depth <= 0) return "N/A";
            switch (info.depth) {
            case 1:  return "1 bpp (Black & White)";
            case 8:  return "8 bpp (256 colors)";
            case 24: return "24 bpp (RGB)";
            case 32: return "32 bpp (RGB + Alpha)";
            default: return QString("%1 bpp").arg(info.depth);
            }
        case 5: return info.format;
        case 6: return info.compression;
        }
    }

    return QVariant();
}

QVariant ImageModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QStringList headers = {
            "File name", "File size", "Size in pixels", "Resolution (dpi)",
            "Color depth", "Format", "Compression"
        };
        return headers.value(section);
    }
    return QVariant();
}

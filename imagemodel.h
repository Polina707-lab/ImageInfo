#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include <QAbstractTableModel>
#include <QFileInfo>
#include <QImageReader>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

struct ImageInfo {
    QString fileName;
    qint64 fileSize;
    QSize dimensions;
    int dpiX, dpiY;
    int depth;
    QString format;
    QString compression;
};

class ImageModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit ImageModel(QObject *parent = nullptr);

    void loadFromFolder(const QString &folderPath);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

signals:
    void loadingFinished(int count);
    void batchLoaded(int count);

private:
    QList<ImageInfo> images;
};

#endif // IMAGEMODEL_H

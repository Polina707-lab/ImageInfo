#include "imagemodel.h"
#include "mainwindow.h"
#include "appstyle.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QMenu>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    qApp->setStyleSheet(AppStyle::styleSheet());

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // === Кнопка File ===
    QPushButton *fileButton = new QPushButton("File", this);
    fileButton->setObjectName("fileButton");
    fileButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(fileButton);

    // === Таблица ===
    view = new QTableView(this);
    model = new ImageModel(this);

    view->setModel(model);
    view->setSortingEnabled(false);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(view, 1);

    // === Кнопка Help ===
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    QPushButton *helpButton = new QPushButton("Help", this);
    helpButton->setObjectName("helpButton");
    bottomLayout->addWidget(helpButton);
    mainLayout->addLayout(bottomLayout);

    setCentralWidget(central);

    // === Статусбар ===
    status = new QStatusBar(this);
    setStatusBar(status);

    folderLabel = new QLabel(this);
    infoLabel = new QLabel(this);
    folderLabel->setStyleSheet("margin-right: 20px;");

    status->addPermanentWidget(folderLabel, 1);
    status->addPermanentWidget(infoLabel, 1);

    // === Действия ===
    actionOpen = new QAction("&Open Folder...", this);
    actionOpen->setShortcut(QKeySequence("Ctrl+O"));
    connect(actionOpen, &QAction::triggered, this, &MainWindow::openFolder);

    actionExport = new QAction("&Export to CSV...", this);
    actionExport->setShortcut(QKeySequence("Ctrl+S"));
    connect(actionExport, &QAction::triggered, this, &MainWindow::exportCSV);

    actionExit = new QAction("E&xit", this);
    actionExit->setShortcut(QKeySequence("Ctrl+Q"));
    connect(actionExit, &QAction::triggered, this, &MainWindow::close);

    addAction(actionOpen);
    addAction(actionExport);
    addAction(actionExit);

    // === Меню по кнопке File ===
    connect(fileButton, &QPushButton::clicked, this, [this]() {
        QMenu menu;
        menu.addAction(actionOpen);
        menu.addAction(actionExport);
        menu.addSeparator();
        menu.addAction(actionExit);
        menu.exec(QCursor::pos());
    });

    // === Help ===
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::showAbout);

    // === Обновление статуса загрузки ===
    connect(model, &ImageModel::batchLoaded, this, [this](int count) {
        infoLabel->setText(QString("Обработано файлов: %1").arg(count));
    });

    connect(model, &ImageModel::loadingFinished, this, [this](int count) {
        infoLabel->setText(QString("Загружено файлов: %1").arg(count));
        actionOpen->setEnabled(true);
        actionExport->setEnabled(true);
    });

    resize(1300, 600);
    setMinimumSize(1300, 600);
    setWindowTitle("Image info");
}

void MainWindow::openFolder() {
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        "Select the folder with images",
        QDir::homePath()
        );

    if (!folderPath.isEmpty()) {
        actionOpen->setEnabled(false);
        actionExport->setEnabled(false);

        model->loadFromFolder(folderPath);

        setWindowTitle(QString("Image info — %1").arg(QFileInfo(folderPath).fileName()));
        folderLabel->setText(QString("Открыта папка: %1").arg(folderPath));
        infoLabel->setText("Загрузка...");
    }
}

void MainWindow::exportCSV() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save CSV", "", "CSV Files (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Не удалось открыть файл для записи");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Заголовки
    QStringList headers;
    for (int col = 0; col < model->columnCount(); ++col) {
        headers << model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
    }
    out << headers.join(";") << "\n";

    // Данные
    for (int row = 0; row < model->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < model->columnCount(); ++col) {
            rowData << model->data(model->index(row, col), Qt::DisplayRole).toString();
        }
        out << rowData.join(";") << "\n";
    }

    file.close();
    infoLabel->setText(QString("CSV сохранён: %1").arg(filePath));
}

void MainWindow::showAbout() {
    QMessageBox::about(this,
                       "About",
                       "<h3>Лабораторная работа 2</h3>"
                       "<p><b>ФИО:</b> Шамрук Полина Александровна<br>"
                       "<b>Курс:</b> 2<br>"
                       "<b>Группа:</b> 11<br>"
                       "<p>Программа считывает информацию об изображениях и выводит её в таблицу.</p>"
                       );
}

MainWindow::~MainWindow() {}

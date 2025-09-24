#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <QLabel>

class ImageModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFolder();
    void showAbout();
    void exportCSV();


private:
    QTableView *view;
    ImageModel *model;
    QStatusBar *status;
    QLabel *folderLabel;
    QLabel *infoLabel;

    QAction *actionOpen;
    QAction *actionExit;
    QAction *actionAbout;
    QAction *actionExport;
};

#endif // MAINWINDOW_H

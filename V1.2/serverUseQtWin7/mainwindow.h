#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "server.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void openVideoShowSlot();
    void showVideoSlot(cv::Mat &frame);
    void showFPSSlot();

public:
    Ui::MainWindow *ui;
    VideoTSServer *pServe;
private:
    bool socketOpenSta;
    int lastFrameCnt;
};

#endif // MAINWINDOW_H

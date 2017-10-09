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
    void pushButtonGoSlot();
    void pushButtonBackSlot();
    void pushButtonLeftSlot();
    void pushButtonRightSlot();
    void pushButtonStopSlot();

public:
    Ui::MainWindow *ui;
    VideoTSServer *pServer;
private:
    bool socketOpenSta;
    int lastFrameCnt;
};

#endif // MAINWINDOW_H

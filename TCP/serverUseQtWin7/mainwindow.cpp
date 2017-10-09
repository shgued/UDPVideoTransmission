#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "server.hpp"
#include <QUdpSocket>

#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QTime>
#include <QString>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    lastFrameCnt = 0;
    socketOpenSta = false;
    pServe = NULL;
    ui->setupUi(this);
    ui->portLineEdit->setText("5136");
    connect(ui->openPushButton,SIGNAL(clicked(bool)),this,SLOT(openVideoShowSlot()));
}

//打开按钮槽
void MainWindow::openVideoShowSlot()
{
    if(socketOpenSta == false){

        socketOpenSta = true;
        pServe = new VideoTSServer;
        qDebug("open button");
        pServe->server_port = ui->portLineEdit->text().toInt();
        if(pServe->server_port > 65535 || pServe->server_port < 1024){
            pServe->server_port = 5136;
            ui->portLineEdit->setText("5136");
        }
        pServe->makeSocket(pServe->server_port);
        ui->portLineEdit->setText(QString::number(pServe->server_port));

        connect(pServe,SIGNAL(picCapTureComplete(cv::Mat &)),this,SLOT(showVideoSlot(cv::Mat &)));
        ui->openPushButton->setDisabled(true);
    }
}

//显示图片
void MainWindow::showVideoSlot(cv::Mat &frame)
{
    QImage image(frame.data,frame.cols,frame.rows,QImage::Format_RGB888);
    ui->videoShowLable->setPixmap(QPixmap::fromImage(image));
}

//每秒计算FPS
void MainWindow::showFPSSlot()
{
    int fps = 0;
    if(pServe != NULL){
        fps = pServe->frameCnt - lastFrameCnt;
        lastFrameCnt = pServe->frameCnt;
    }
    ui->fpsLabel->setText(QString("fps:").append(QString::number(fps)));

}

MainWindow::~MainWindow()
{
    delete pServe;
    delete ui;
}

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
    pServer = NULL;
    ui->setupUi(this);
    ui->portLineEdit->setText("5136");
    connect(ui->openPushButton,SIGNAL(clicked(bool)),this,SLOT(openVideoShowSlot()));
    connect(ui->pushButtonGo,SIGNAL(clicked(bool)),this,SLOT(pushButtonGoSlot()));
    connect(ui->pushButtonBack,SIGNAL(clicked(bool)),this,SLOT(pushButtonBackSlot()));
    connect(ui->pushButtonLeft,SIGNAL(clicked(bool)),this,SLOT(pushButtonLeftSlot()));
    connect(ui->pushButtonRight,SIGNAL(clicked(bool)),this,SLOT(pushButtonRightSlot()));
    connect(ui->pushButtonStop,SIGNAL(clicked(bool)),this,SLOT(pushButtonStopSlot()));
}

void MainWindow::openVideoShowSlot()
{
    if(socketOpenSta == false){
        socketOpenSta = true;
        pServer = new VideoTSServer;
        qDebug("open button");
        pServer->server_port = ui->portLineEdit->text().toInt();
        if(pServer->server_port > 65535 || pServer->server_port < 1024){
            pServer->server_port = 5136;
            ui->portLineEdit->setText("5136");
        }
        pServer->makeSocket(pServer->server_port);

        connect(pServer,SIGNAL(picCapTureComplete(cv::Mat &)),this,SLOT(showVideoSlot(cv::Mat &)));
        ui->openPushButton->setDisabled(true);
    }
}

void MainWindow::showVideoSlot(cv::Mat &frame)
{
    QImage image(frame.data,frame.cols,frame.rows,QImage::Format_RGB888);
    ui->videoShowLable->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::showFPSSlot()
{
    int fps = 0;
    if(pServer != NULL){
        pServer->fps = pServer->frameCnt - lastFrameCnt;
        lastFrameCnt = pServer->frameCnt;
        fps = pServer->fps;
    }

    ui->fpsLabel->setText(QString("fps:").append(QString::number(fps)));
}

void MainWindow::pushButtonGoSlot()
{
    pServer->gbStatus++;
    if(pServer->gbStatus > pServer->GB_STATUS_MAX_VALUE)
        pServer->gbStatus = pServer->GB_STATUS_MAX_VALUE;
}

void MainWindow::pushButtonBackSlot()
{
    pServer->gbStatus--;
    if(pServer->gbStatus < -pServer->GB_STATUS_MAX_VALUE)
        pServer->gbStatus = -pServer->GB_STATUS_MAX_VALUE;
}

void MainWindow::pushButtonLeftSlot()
{
    pServer->lrStatus--;
    if(pServer->lrStatus < -pServer->LR_STATUS_MAX_VALUE)
        pServer->lrStatus = -pServer->LR_STATUS_MAX_VALUE;
}

void MainWindow::pushButtonRightSlot()
{
    pServer->lrStatus++;
    if(pServer->lrStatus > pServer->LR_STATUS_MAX_VALUE)
        pServer->lrStatus = pServer->LR_STATUS_MAX_VALUE;
}

void MainWindow::pushButtonStopSlot()
{
    pServer->lrStatus = 0;
    pServer->gbStatus = 0;
}


MainWindow::~MainWindow()
{
    delete pServer;
    delete ui;
}

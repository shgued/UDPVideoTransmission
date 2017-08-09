//============================================================================
// Name        : Server.cpp
// Author      : shgued
// Version     : 1.0
// Copyright   : open source
// Description : OpenCV experimentation in C++, transform video with UDP
//============================================================================
#include "server.hpp"
#include <QtNetwork>
#include <QDebug>
#include <QApplication>
#include <QObject>
#include "ui_mainwindow.h"
#include "mainwindow.h"

#define TAG "Server- "

using namespace std;
using namespace cv;

VideoTSServer::VideoTSServer(QObject *parent) :  QObject(parent)
{
    frame = Mat(PIC_HIGHT,PIC_WIDRH,CV_8UC3);//set by pivture size
    frameCnt = 0;
    server_port = 5136;
    udpSocket = NULL;
    recvCnt = 0;
    pkgSta = true;
    lastPkgIndex = SEND_PKG_NUM_PER_PICTURE-1;
}
VideoTSServer::~VideoTSServer(){
    udpSocket->close();
}

void VideoTSServer::getPkgHead(struct PkgHeadStr &pkgHead){
    if(pkgDataGram.size() >= 6){
        unsigned char *pkgData = (unsigned char *)pkgDataGram.data();
        pkgHead.index = pkgData[0];
        pkgHead.sum = pkgData[1]|(pkgData[2]<<8);
        pkgHead.size = pkgData[3]|(pkgData[4]<<8)|(pkgData[5]<<8);
    }
}

void VideoTSServer::processPendingDatagrams()
{
    int key,cnt;
    int i,sum;

    while (udpSocket->hasPendingDatagrams()) {
        pkgDataGram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(pkgDataGram.data(), pkgDataGram.size());

        //statusLabel->setText(tr("Received datagram: \"%1\"").arg(datagram.data()));
    }
    //if(recvCnt%(SEND_PKG_NUM_PER_PICTURE*100) == 0)\
        qDebug << QString("recvCnt:") << recvCnt << endl;
    recvCnt++;

    getPkgHead(pkgHead);
    cnt = pkgDataGram.size();
    if(recvCnt%(SEND_PKG_NUM_PER_PICTURE*100) == 0)
        qDebug("recv:%d index:%d size:%d",recvCnt,pkgHead.index,pkgHead.size);

    if(cnt == pkgHead.size){
        //cout << "recIndex:" << pkgHead.index << endl;

        k = pkgHead.size;
        unsigned char *pkgData = (unsigned char *)pkgDataGram.data();
        for(i=6,sum=0;i<k;i++){
            dataEncode.push_back(pkgData[i]);
            sum += pkgData[i];
        }
        if(sum%65536 != pkgHead.sum) cout << "pkg sum error" << sum <<endl;
        k = pkgHead.index - 1;
        if(k < 0) k = SEND_PKG_NUM_PER_PICTURE - 1;
        if(k != lastPkgIndex){
            cout << "last:" << lastPkgIndex <<" new:" << pkgHead.index <<endl;
            pkgSta = false;
        }
        lastPkgIndex = pkgHead.index;

        if(pkgHead.index == (SEND_PKG_NUM_PER_PICTURE-1)){
            if(pkgSta){
                frame = imdecode(dataEncode, CV_LOAD_IMAGE_COLOR);

                //imshow("CameraCapture", frame); //by opencv video show

                emit picCapTureComplete(frame);
                frameCnt++;
                //qDebug("emit");
            }
            else
                qDebug("error frame!");
            //key = waitKey(1);//bug
            dataEncode.clear();
            pkgSta = true;
        }
    }
    else
        cout<<"pkg size error! cnt:"<< cnt << pkgHead.size <<endl;
}

void VideoTSServer::makeSocket(quint16 port){

    udpSocket = new QUdpSocket();
    udpSocket->bind(port, QUdpSocket::ShareAddress);
    qDebug("port:%d",port);
    QObject::connect(udpSocket, SIGNAL(readyRead()),this, SLOT(processPendingDatagrams()));
}

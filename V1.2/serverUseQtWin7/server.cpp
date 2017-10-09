//============================================================================
// Name        : Server.cpp
// Author      : shgued
// Version     : 1.2
// Copyright   : open source
// Description : OpenCV experimentation in C++, transform video with UDP
// !! 标记重点关注
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

//获取包头
void VideoTSServer::getPkgHead(struct PkgHeadStr &pkgHead){
    if(pkgDataGram.size() >= PACKET_HEAD_LENGTH){
        unsigned char *pkgData = (unsigned char *)pkgDataGram.data();
        pkgHead.index = pkgData[0];
        pkgHead.sum = pkgData[1]|(pkgData[2]<<8);
        pkgHead.size = pkgData[3]|(pkgData[4]<<8)|(pkgData[5]<<8);
    }
}

//从socket 读取数据合成图片，图片格式转换
void VideoTSServer::processPendingDatagrams()
{
    int key,cnt;
    int i,sum;

    while (udpSocket->hasPendingDatagrams()) {
        //cout << udpSocket->pendingDatagramSize() << endl;
        pkgDataGram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(pkgDataGram.data(), pkgDataGram.size());
    }
    //if(recvCnt%(SEND_PKG_NUM_PER_PICTURE*100) == 0)   qDebug << QString("recvCnt:") << recvCnt << endl;
    recvCnt++;

    getPkgHead(pkgHead);
    cnt = pkgDataGram.size();

    if(recvCnt%(SEND_PKG_NUM_PER_PICTURE*100) == 0)
        qDebug("recv:%d index:%d size:%d",recvCnt,pkgHead.index,pkgHead.size);

    if(cnt == pkgHead.size){
        //cout << "recIndex:" << pkgHead.index << endl;

        k = pkgHead.size;
        unsigned char *pkgData = (unsigned char *)pkgDataGram.data();
        for(i=PACKET_HEAD_LENGTH,sum=0;i<k;i++){
            dataEncode.push_back(pkgData[i]);
            sum += pkgData[i];
        }
        //发送数据包和校验，未发现过错误
        if(sum%65536 != pkgHead.sum) cout << "pkg sum error" << sum <<endl;

        k = pkgHead.index - 1;
        if(k < 0) k = SEND_PKG_NUM_PER_PICTURE - 1;
        if(k != lastPkgIndex){
            pkgSta = false;
            cout << "pkg last:" << lastPkgIndex << "new:" << k <<endl;
        }
        lastPkgIndex = pkgHead.index;

        if(pkgHead.index == (SEND_PKG_NUM_PER_PICTURE-1)){
            if(pkgSta){
                frame = imdecode(dataEncode, CV_LOAD_IMAGE_COLOR);

                //imshow("CameraCapture", frame); //by opencv video 尝试一下

                //将pencv BGR格式传为显示用的RGB格式 !!
                if(frame.elemSize() == 3){//BGR888型，3字节
                    int  m,n;
                    unsigned char *dat = frame.data,ch;
                    n =  frame.rows*frame.cols;
                    //if(0)
                    for(m=0;m<n;m++){
                       ch = *dat;
                       *dat = *(dat+2);
                       *(dat+2) = ch;
                       dat += 3;
                    }
                    //cout << "r:" << frame.rows << "c:" << frame.cols << "n:" << n << endl;
                }
                emit picCapTureComplete(frame); //发送信号，用于触发qt显示图片
                frameCnt++;
            }
            else
                qDebug("error frame!");
            //key = waitKey(1);//bug，显示图片会卡住
            dataEncode.clear();
            pkgSta = true;
        }
    }
    else
        cout<<"pkg size error! cnt:"<< cnt << pkgHead.size <<endl;
}

//打开UDP socket
void VideoTSServer::makeSocket(quint16 port){

    udpSocket = new QUdpSocket();
    udpSocket->bind(port, QUdpSocket::ShareAddress);
    qDebug("port:%d",port);
    QObject::connect(udpSocket, SIGNAL(readyRead()),this, SLOT(processPendingDatagrams()));
}

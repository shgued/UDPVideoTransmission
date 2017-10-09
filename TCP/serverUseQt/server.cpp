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

#define PACKAGE_HEAD_LENGTH 10 //包头
#define TCPSOCKET_PACKAGE_DATA_NUM (1448-PACKAGE_HEAD_LENGTH) //除去包头最大数据量
#define MAX_TCP_PACKAGE_SIZE (TCPSOCKET_PACKAGE_DATA_NUM + PACKAGE_HEAD_LENGTH)//最大数据包长度

#define TAG "Server- "

using namespace std;
using namespace cv;

VideoTSServer::VideoTSServer(QObject *parent) :  QObject(parent)
{
    frameCnt = 0;
    server_port = 5136;
    recvCnt = 0;
    pkgSta = true;
    byteArrayPos = 0;
}

VideoTSServer::~VideoTSServer(){
    tcpServer->close();
    clientConnection->close();
}

//获取包头
void VideoTSServer::getPkgHead(struct PkgHeadStr &ph,unsigned char *dat){
    ph.index = dat[0] | (dat[1]<<8);
    ph.totalNum = dat[2] | (dat[3]<<8) | (dat[4]<<16);
    ph.photoCnt = dat[5];
    //cout << (int) dat[0] << "  " <<  (int) dat[1] <<"  " <<  (int) dat[2] << "  " <<  (int) dat[3]  << "  " <<  (int) dat[4] <<  "  " <<  (int) dat[5] << endl;
}

//寻找下一个没有存储数据的byteArray，存放TCP收到的数据
int VideoTSServer::findNextArrayPos(void){
    int i;
    int cnt = 0;
    for(i=byteArrayPos;;){
        if(i>=byteArrayNum) //循环查找
            i -= byteArrayNum;
        if(byteArray[i].size()>0){
            i++;
            cnt++;
            //所有位置都有数据，从上次接收的下一位置存放新接收的数据
            if(cnt>=byteArrayNum){
//                byteArrayPos++;
//                if(byteArrayPos>=byteArrayNum)
//                    byteArrayPos -= byteArrayNum;
                return byteArrayPos;
            }
        }
        else{
            byteArrayPos = i+1;//待存储位置
            if(byteArrayPos>=byteArrayNum) byteArrayPos -= byteArrayNum;
            return i;
        }
    }
}

//检查有没有收到完整的图片数据，有则对图片解压缩并发送显示图片信号
void VideoTSServer::checkShow(void){
    int i,j,k;
    int index;
    recvCnt++;

    for(i=0;i<photoInfoNum;i++){
        //if(recvCnt % 100 == 0) cout << "photoInfo[" << i << "].totoalNum:" <<photoInfo[i].totoalNum <<endl;
        //收到完整图片
        if(photoInfo[i].totoalNum && photoInfo[i].totoalNum == photoInfo[i].cnt){
            dataEncode.clear();
            dataEncode.resize(photoInfo[i].totoalNum);
            int pos;
            //合并压缩图片数据
            for(j=0;j<photoInfo[i].sIndex.size();j++){
                pos = photoInfo[i].sIndex[j].second * TCPSOCKET_PACKAGE_DATA_NUM;
                if(pos > photoInfo[i].totoalNum) continue;
                index = photoInfo[i].sIndex[j].first;
                //cout << photoInfo[i].totoalNum <<"index" << photoInfo[i].sIndex[j].second << " " << byteArray[index].size() <<endl;
                int size = byteArray[index].size();
                for(k=PACKAGE_HEAD_LENGTH;k<size;k++){
                    dataEncode[pos++] = byteArray[index][k];
                }
            }
            //清除相应接收缓冲区
            for(j=0;j<photoInfo[i].sIndex.size();j++){
                byteArray[photoInfo[i].sIndex[j].first].clear();
            }
            photoInfo[i].totoalNum = 0;
            photoInfo[i].cnt = 0;
            photoInfo[i].sIndex.clear();

            frame = imdecode(dataEncode, CV_LOAD_IMAGE_COLOR);
            //cout << "wit:" << frame.cols << "he:" << frame.rows << endl;
            //if(frame.cols && frame.rows)
            //imshow("CameraCapture", frame); //by opencv video show

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

            emit picCapTureComplete(frame);
            frameCnt++;
            //cout <<"frame CNT:" <<  frameCnt <<endl;
            //qDebug("emit");
        }
    }
}
//处理TCP socket 接收数据
void VideoTSServer::processPendingDatagrams()
{
    int i,j,m,n,size,pos,pos2;

    tcpData.append(clientConnection->readAll());

    m = 0;//已解析到的数据位置
    while(m<tcpData.size()-4){
    //{
        //cout << tcpData.size();
        size = tcpData.size()-4;
        unsigned char *pch = (unsigned char *)tcpData.data();
        int findSta = 0;
        for(m=0;m<size;m++){
            //cout << m << endl;
            if(*(unsigned short *)pch == 0xaaaa){
                //if(pch[0] == 0xaa && pch[1] == 0xaa){
                int len = pch[2] | (pch[3]<<8);//数据包长度
                //if(len == 1448)
                //cout << "len=" << len << " "<<endl;
                //0xaaaa 作为数据包的特殊标记
                if(len<=MAX_TCP_PACKAGE_SIZE && len>0 &&  m+len<=tcpData.size()-2 && (pch[len] == 0xaa && pch[len+1] == 0xaa)){
                       pos = findNextArrayPos();
                       //cout << "tcpdata 1  " << tcpData.size() << endl;
                       byteArray[pos] = tcpData.mid(m-PACKAGE_HEAD_LENGTH+4,len);
                       tcpData.remove(m-PACKAGE_HEAD_LENGTH+4,len);
                       //cout << "pos:" << pos <<  "  " << byteArray[pos].size() << endl;
                       //cout << "tcpdata 2  " << tcpData.size() << endl << endl;
                       getPkgHead(pkgHead,(unsigned char*)byteArray[pos].data());
                       //cout << pos  << "  " <<  byteArray[pos].size() << "  "<< pkgHead.index << "  " << pkgHead.totalNum << " " << byteArray[pos].at(0)<< endl;
                       int cnt = photoInfoNum;
                       i = lastPhotoPos;
                       while(cnt){
                           //cout << lastPhotoPos << "  " << i << " " << pkgHead.totalNum <<  " << " << photoInfo[i].totoalNum << endl;
                           if(pkgHead.totalNum == photoInfo[i].totoalNum && pkgHead.photoCnt == photoInfo[i].photoCnt){
                               photoInfo[i].sIndex.push_back(make_pair(pos,pkgHead.index));
                               photoInfo[i].cnt += byteArray[pos].size()-PACKAGE_HEAD_LENGTH;
//                               if(byteArray[pos].size() != MAX_TCP_PACKAGE_SIZE)
//                                   cout << i << " 11  " << photoInfo[i].cnt << "  "<< byteArray[pos].size()<< "  "<< pkgHead.totalNum << endl;
//                               if(photoInfo[i].cnt >= photoInfo[i].totoalNum)
//                                   cout << i << " 22  "   << photoInfo[i].cnt << " " << photoInfo[i] .photoCnt << "  "<< pkgHead.totalNum << endl;
                               lastPhotoPos = i;
                               break;
                           }
                           i++;
                           i %= photoInfoNum;
                           cnt--;
                       }
                       if(cnt == 0){//new photo or other command
                           cnt = photoInfoNum;
                           i = lastPhotoPos;
                           while(cnt){
                               //cout << lastPhotoPos << "  " << i << " " << pkgHead.totalNum <<  " 2 < " << photoInfo[i].totoalNum <<" " << photoInfo[i].cnt << " " << photoInfo[i].totoalNum-photoInfo[i].cnt << endl;
                               if(photoInfo[i].sIndex.empty()){
                                   photoInfo[i].sIndex.push_back(make_pair(pos,pkgHead.index));
                                   photoInfo[i].totoalNum = pkgHead.totalNum;
                                   photoInfo[i].cnt = byteArray[pos].size()-PACKAGE_HEAD_LENGTH;
                                   photoInfo[i].photoCnt = pkgHead.photoCnt;
                                   //cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << photoInfo[i].cnt << endl;
                                   break;
                               }
                               i++;
                               i %= photoInfoNum;
                               cnt--;
                           }

                           if(cnt == 0){
                               lastPhotoPos++;
                               lastPhotoPos %= photoInfoNum;
                               i = lastPhotoPos;
                               cout << lastPhotoPos << "  " << i << " " << pkgHead.totalNum <<  " 2 < " << photoInfo[i].totoalNum <<" " << photoInfo[i].cnt << " " << photoInfo[i].totoalNum-photoInfo[i].cnt << endl;
                               photoInfo[lastPhotoPos].sIndex.clear();
                               photoInfo[lastPhotoPos].sIndex.push_back(make_pair(pos,pkgHead.index));
                               photoInfo[lastPhotoPos].totoalNum = pkgHead.totalNum;
                               photoInfo[i].photoCnt = pkgHead.photoCnt;
                               photoInfo[lastPhotoPos].cnt = byteArray[pos].size()-PACKAGE_HEAD_LENGTH;
                           }
                       }
                       else
                           checkShow();
                       findSta = 1;
                   }
                   if(findSta == 1) break;
             }

            if(findSta == 1) break;
            pch++;
        }
       // cout << "m:" << m <<  " " << tcpData.size() << endl;
    }
    //无法解析的数据量比两个最大包长度大，留下一个最大包长度数据量
    if(m >= 2*MAX_TCP_PACKAGE_SIZE+PACKAGE_HEAD_LENGTH) tcpData.remove(0,tcpData.size() - MAX_TCP_PACKAGE_SIZE);
}

//建立与客户端通信的socket
void VideoTSServer::sendFortune()
{
    clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, &QAbstractSocket::disconnected,clientConnection, &QObject::deleteLater);

    QObject::connect(clientConnection, SIGNAL(readyRead()),this, SLOT(processPendingDatagrams()));
}

//创建 TCP server socket
void VideoTSServer::makeSocket(quint16 port){

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen()) {
        qDebug("unable listen");
        //tcpServer->close();
        return;
    }

    //qDebug("port:%d",port);
    qDebug("port:%d",tcpServer->serverPort());
    server_port = tcpServer->serverPort();
    connect(tcpServer, &QTcpServer::newConnection, this, &VideoTSServer::sendFortune);
    //namedWindow("CameraCapture", 0);
}

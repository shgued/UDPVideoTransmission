//============================================================================
// Name        : Server.cpp
// Author      : shgued
// Version     : 2.0
// Copyright   : open source
// Description : OpenCV experimentation in C++, transform video with UDP
// !! 标记重点关注
// udp加入包排序和丢包重发控制(只对当前正在接收和上一张图片丢失的包发送重发请求)
//============================================================================

#include "server.hpp"
#include <QtNetwork>
#include <QDebug>
#include <QApplication>
#include <QObject>
#include "ui_mainwindow.h"
#include "mainwindow.h"

using namespace std;
using namespace cv;

#define TAG "Server- "
#define  PKG_RESEND_TEST  //测试丢包重发功能，定义则测试 ，忽略第一次接收的pkgIndex == 1的包

//---------客户端服务器端设置要一致
#define PACKAGE_HEAD_LENGTH 6 //包头长度
#define MAX_PACKAGE_SIZE (1460*2)//最大数据包长度
#define MAX_PACKAGE_DATA_NUM (MAX_PACKAGE_SIZE-PACKAGE_HEAD_LENGTH) //除去包头最大数据量
//---------客户端服务器端设置要一致

//数据包头结构
struct PkgHeadStr{
    unsigned short picIndex;//0~99图片索引
    unsigned short pkgIndex;//0～255数据包索引
    unsigned int size;//一幅图片数据量
};
struct PkgHeadStr pkgHead;//包头

VideoTSServer::VideoTSServer(QObject *parent) :  QObject(parent)
{
    frameCnt = 0;
    server_port = 5136;
    udpSocket = NULL;
    clientIp = 0;
    clientPort = 0;
    recvCnt = 0;
    pkgSta = true;

    lrStatus = 0;
    gbStatus = 0;

    //参数设置，单位：ms
    transmitDelayTime = 20;//数据包传输往返时间
    randomOrderDelayTime = 10;//乱序间隔时间
    timerCnt = 0;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    timer->start(1);//1m周期
}

VideoTSServer::~VideoTSServer(){
    udpSocket->close();
}

//解析包头
struct PkgHeadStr getPkgHead(unsigned char *pkgData){
    struct PkgHeadStr ph;
    ph.picIndex = pkgData[0];
    ph.pkgIndex = pkgData[1];
    ph.size = pkgData[2] | (pkgData[3]<<8) | (pkgData[4]<<16) | (pkgData[5]<<24);
    return ph;
}

//检查有没有收到完整的图片数据，有则对图片解压缩并发送显示图片信号
void VideoTSServer::checkShow(void){
    int i;
    recvCnt++;
   //cout << "checkshow start" << endl;

    for(i=0;i<photoInfoNum;i++){
        //if(recvCnt % 100 == 0) cout << "photoInfo[" << i << "].totoalNum:" <<photoInfo[i].totoalNum <<endl;
        //收到完整图片
        if(photoInfo[i].totoalNum && photoInfo[i].totoalNum == photoInfo[i].cnt){            
            frame = imdecode(photoInfo[i].data, CV_LOAD_IMAGE_COLOR);
            //cout << "wit:" << frame.cols << "he:" << frame.rows << endl;
            //if(frame.cols && frame.rows)
            //imshow("CameraCapture", frame); //by opencv video show

            //清除相应接收缓冲区
            photoInfo[i].data.clear();
            photoInfo[i].sIndex.clear();
            photoInfo[i].totoalNum = 0;
            photoInfo[i].cnt = 0;
            photoInfo[i].maxPkgCnt = 0;

            //将pencv BGR格式传为显示用的RGB格式 !!
            if(frame.elemSize() == 3){//BGR888型，3字节
                int  m,n;
                unsigned char *dat = frame.data,ch;
                n =  frame.rows*frame.cols;
                for(m=0;m<n;m++){
                   ch = *dat;
                   *dat = *(dat+2);
                   *(dat+2) = ch;
                   dat += 3;
                }
                //cout << "r:" << frame.rows << "c:" << frame.cols << "n:" << frame.isContinuous() << endl;
            }
            else cout << "frame error!" << endl;

            emit picCapTureComplete(frame);//出发QT显示图片
            //cout << "show picIndex" << photoInfo[i].picIndex << endl;

            frameCnt++;
            //cout <<"frame CNT:" <<  frameCnt <<endl;
            //qDebug("emit");
        }
    }
    //cout << "checkshow end" << endl;
}

//产生重发请求
void VideoTSServer::pkgResendRequest()
{
    int i,j;
    int size;
    struct SingleResendInfoStr info;

    for(i=0;i<photoInfoNum;i++){
        if(getDiff(photoInfo[i].picIndex,picRecvIndex,MAX_PIC_INDEX) > 1){
            continue;
        }
        size = photoInfo[i].sIndex.size();

        for(j=0;j<size;j++){
            if(photoInfo[i].sIndex[j].sta == NOT_RECV_DATA){
                if(j < photoInfo[i].maxPkgCnt && getDiff(photoInfo[i].sIndex[j].time,newRecvTime,MAX_TIMER_CNT_VAL) > randomOrderDelayTime){
                    //cout << " photoInfo[i].maxPkgCnt:" <<  photoInfo[i].maxPkgCnt << endl;
                    photoInfo[i].sIndex[j].sta = RESEDN_REQUEST;
                    photoInfo[i].sIndex[j].time = newRecvTime;
                    info.picIndex = photoInfo[i].picIndex;
                    info.pkgIndex = j;
                    info.size = photoInfo[i].totoalNum;
                    resendInfo.push_back(info);
                }
            }
            else if(photoInfo[i].sIndex[j].sta == RESEDN_REQUEST){//已经重发过了
                if(getDiff(photoInfo[i].sIndex[j].time,newRecvTime,MAX_TIMER_CNT_VAL) > transmitDelayTime){
                    //cout << " diff:" <<  getDiff(photoInfo[i].sIndex[j].time,newRecvTime,MAX_TIMER_CNT_VAL) << endl;
                    photoInfo[i].sIndex[j].time = newRecvTime;
                    info.picIndex = photoInfo[i].picIndex;
                    info.pkgIndex = j;
                    info.size = photoInfo[i].totoalNum;
                    resendInfo.push_back(info);
                }
            }
        }
    }
}

//从socket 读取数据合成图片，图片格式转换
void VideoTSServer::processPendingDatagrams()
{
    int cnt;
    int i;
//cout << "processPendingDatagrams start" << endl;
    while (udpSocket->hasPendingDatagrams()) {
        newRecvTime = timerCnt;
        pkgDataGram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(pkgDataGram.data(), pkgDataGram.size(),&clientIp,&clientPort);
        //if(recvCnt % 20 == 0) cout << clientIp.toString().toStdString().data() << "   " << clientPort << endl;

        pkgHead = getPkgHead((unsigned char *)pkgDataGram.data());
        cnt = pkgDataGram.size();

        //cout << "timeCnt:" << timerCnt << " recvIndex:" << (int)pkgHead.pkgIndex<< endl;
        //cout << "recvIndex:" << (int)pkgHead.pkgIndex << " cnt:" << cnt << endl;

        pkgHead = getPkgHead((unsigned char*)pkgDataGram.data());
        //显示数据包信息
        //cout <<  pkgDataGram.size() << " pkg:"<< pkgHead.pkgIndex << " pic:" << pkgHead.picIndex << " size:" << pkgHead.size << endl;

        int cnt = photoInfoNum;
        i = lastPhotoPos;
        while(cnt){
           //cout << lastPhotoPos << "  " << i << " " << pkgHead.size <<  " << " << photoInfo[i].totoalNum << endl;
           if(pkgHead.size == photoInfo[i].totoalNum && pkgHead.picIndex == photoInfo[i].picIndex){//信息匹配
                if(pkgHead.pkgIndex < photoInfo[i].sIndex.size()){
                    if(photoInfo[i].sIndex[pkgHead.pkgIndex].sta==NOT_RECV_DATA||photoInfo[i].sIndex[pkgHead.pkgIndex].sta==RESEDN_REQUEST){

                        #ifdef PKG_RESEND_TEST  //测试丢包重发功能
                            //忽略 pkgIndex == 1 的包
                            if(pkgHead.pkgIndex!=1 || photoInfo[i].sIndex[pkgHead.pkgIndex].sta!=NOT_RECV_DATA){
                                if(pkgDataGram.size()<=MAX_PACKAGE_SIZE && \
                                        pkgHead.pkgIndex*MAX_PACKAGE_DATA_NUM+pkgDataGram.size()-PACKAGE_HEAD_LENGTH <= photoInfo[i].totoalNum){
                                    memcpy(photoInfo[i].data.data()+pkgHead.pkgIndex*MAX_PACKAGE_DATA_NUM,\
                                            pkgDataGram.data()+PACKAGE_HEAD_LENGTH,pkgDataGram.size()-PACKAGE_HEAD_LENGTH);
                                }
                                else cout << "not photo data 1!" <<pkgDataGram.size() << endl;
                                photoInfo[i].sIndex[pkgHead.pkgIndex].time = newRecvTime;
                                photoInfo[i].sIndex[pkgHead.pkgIndex].sta = HAS_RECV_DATA;
                                photoInfo[i].cnt += pkgDataGram.size()-PACKAGE_HEAD_LENGTH;
                                if(pkgHead.pkgIndex > photoInfo[i].maxPkgCnt)
                                    photoInfo[i].maxPkgCnt = pkgHead.pkgIndex;
                            }
                        #else
                            //photoInfo[i].sIndex[pkgHead.pkgIndex].pkgPos = pos;
                            if(pkgDataGram.size()<=MAX_PACKAGE_SIZE && \
                                    pkgHead.pkgIndex*MAX_PACKAGE_DATA_NUM+pkgDataGram.size()-PACKAGE_HEAD_LENGTH <= photoInfo[i].totoalNum){
                                memcpy(photoInfo[i].data.data()+pkgHead.pkgIndex*MAX_PACKAGE_DATA_NUM,\
                                        pkgDataGram.data()+PACKAGE_HEAD_LENGTH,pkgDataGram.size()-PACKAGE_HEAD_LENGTH);
                            }
                            else cout << "not photo data!" << endl;
                            photoInfo[i].sIndex[pkgHead.pkgIndex].time = newRecvTime;
                            photoInfo[i].sIndex[pkgHead.pkgIndex].sta = HAS_RECV_DATA;
                            photoInfo[i].cnt += pkgDataGram.size()-PACKAGE_HEAD_LENGTH;
                            if(pkgHead.pkgIndex > photoInfo[i].maxPkgCnt)
                                photoInfo[i].maxPkgCnt = pkgHead.pkgIndex;
                        #endif
                    }
                }
                else
                    cout << "pkg param error! sta:" << photoInfo[i].sIndex[pkgHead.pkgIndex].sta \
                         << " index:" <<pkgHead.pkgIndex << " size:" << photoInfo[i].sIndex.size() << endl;
                lastPhotoPos = i;
                break;
           }
           i++;
           i %= photoInfoNum;
           cnt--;
        }
        if(cnt == 0){//新的图片
           cnt = photoInfoNum;
           i = lastPhotoPos;
           while(cnt){
               //cout << lastPhotoPos << "  " << i << " " << pkgHead.size <<  " 2 < " << photoInfo[i].totoalNum <<" " << photoInfo[i].cnt << " " << photoInfo[i].totoalNum-photoInfo[i].cnt << endl;
               if(photoInfo[i].sIndex.empty()){                   
                   //cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$:" << i << endl;
                   break;
               }
               i++;
               i %= photoInfoNum;
               cnt--;
           }

           if(cnt == 0){ //缓存满了
               lastPhotoPos++;
               lastPhotoPos %= photoInfoNum;
               i = lastPhotoPos;
//               cout <<"full:" ;
//               for(int j=0;j<photoInfoNum;j++)
//                   cout << " info" << j <<":picIndex:" << photoInfo[j].picIndex;
//               cout << endl;
               photoInfo[i].sIndex.clear();
               photoInfo[i].data.clear();
           }

           if(pkgDataGram.size()<=MAX_PACKAGE_SIZE && \
                   pkgHead.pkgIndex*MAX_PACKAGE_DATA_NUM+pkgDataGram.size()-PACKAGE_HEAD_LENGTH <= pkgHead.size){
               int num;//分包数量
               num = pkgHead.size/MAX_PACKAGE_DATA_NUM;
               if(pkgHead.size > num*MAX_PACKAGE_DATA_NUM) num++;
               photoInfo[i].sIndex.resize(num);
               //init
               for(int p=0;p<num;p++){
                   photoInfo[i].sIndex[p].sta = NOT_RECV_DATA;
                   photoInfo[i].sIndex[p].time = newRecvTime;
               }
               photoInfo[i].sIndex[pkgHead.pkgIndex].sta = HAS_RECV_DATA;
               photoInfo[i].totoalNum = pkgHead.size;
               photoInfo[i].data.resize(photoInfo[i].totoalNum);

               memcpy(photoInfo[i].data.data()+pkgHead.pkgIndex*MAX_PACKAGE_DATA_NUM,\
                       pkgDataGram.data()+PACKAGE_HEAD_LENGTH,pkgDataGram.size()-PACKAGE_HEAD_LENGTH);

               photoInfo[i].cnt = pkgDataGram.size()-PACKAGE_HEAD_LENGTH;
               photoInfo[i].picIndex = pkgHead.picIndex;
               photoInfo[i].maxPkgCnt = pkgHead.pkgIndex;
           }
           else cout << "not photo data 0!" <<pkgDataGram.size() << endl;

           if(getDiff(picRecvIndex,pkgHead.picIndex,MAX_PIC_INDEX) < (int)(0.8*MAX_PIC_INDEX)){
               picRecvIndex = pkgHead.picIndex;
           }

           lastPkgPicIndex = pkgHead.picIndex;
        }

        checkShow();
        pkgResendRequest();//产生重发请求
        lastRecvTime = newRecvTime;
    }
//    cout << "processPendingDatagrams end" << endl;
}

void VideoTSServer::timerUpdate() //1ms
{
    int i,j,size;
    char dat[1600];
    timerCnt++;
    if(timerCnt >= MAX_TIMER_CNT_VAL) timerCnt = 0;
//cout << "timerUpdate start" << endl;
    if(timerCnt%50==0 && clientPort != 0 && udpSocket != NULL){
        dat[0] = 0x78;
        dat[1] = 0x56;
        dat[2] = 0x34;
        dat[3] = 0x12;
        dat[4] = CAR_CTL_CMD_ID;
        dat[5] = gbStatus;
        dat[6] = lrStatus;
        udpSocket->writeDatagram(dat,7,clientIp, clientPort);
        //cout << "gb:" << gbStatus << " lr:" << lrStatus << endl;
    }
    //if(0)
    if(resendInfo.size() != 0){ //重发请求
        //cout << "size:" << resendInfo.size() << endl;
        dat[0] = 0x78;
        dat[1] = 0x56;
        dat[2] = 0x34;
        dat[3] = 0x12;
        dat[4] = PKG_RESEND_CMD_ID;
        dat[5] = fps;//帧率
        for(i=0,j=6;i<resendInfo.size();i++){
            dat[j++] = resendInfo[i].picIndex;
            dat[j++] = resendInfo[i].pkgIndex;
            size = resendInfo[i].size;
            dat[j++] = size&0xff;
            size >>= 8;
            dat[j++] = size&0xff;
            size >>= 8;
            dat[j++] = size&0xff;
            size >>= 8;
            dat[j++] = size&0xff;
            //cout << "resendRequest: picIndex:" << resendInfo[i].picIndex << " newest picIndex:" << picRecvIndex << " pkgIndex:" << resendInfo[i].pkgIndex << endl;
        }
        resendInfo.clear();
        udpSocket->writeDatagram(dat,j,clientIp, clientPort);
    }
 //   cout << "timerUpdate end" << endl;
}

//计算差值 last：较早值 now：新值 max:可取最大值
int VideoTSServer::getDiff(int last, int now, int max)
{
    int ans;
    ans = now - last;
    if(ans < 0) ans += max;
    return ans;
}


//打开UDP socket
void VideoTSServer::makeSocket(quint16 port){

    udpSocket = new QUdpSocket();
    udpSocket->bind(port, QUdpSocket::ShareAddress);
    qDebug("port:%d",port);
    QObject::connect(udpSocket, SIGNAL(readyRead()),this, SLOT(processPendingDatagrams()));
}

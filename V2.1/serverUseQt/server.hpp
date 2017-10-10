#ifndef __server_hpp_
#define __server_hpp_

//#include <opencv2/opencv.hpp>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/videoio/videoio.hpp>

#include <time.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h> //read & write g++
#include <vector>

#include <QUdpSocket>
#include <QWidget>
#include <QTime>

using namespace cv;
using namespace std;

class VideoTSServer : public QObject
{
    Q_OBJECT
public:
    VideoTSServer(QObject *parent = 0);
    ~VideoTSServer();

    void makeSocket(quint16 port);

    char server_ip[16]; //stream sent android camera
    int server_port;
    cv::Mat frame;//picture frame
    int frameCnt;
    int fps;//每秒帧率

    short lrStatus;//左右转状态 >0 右
    short gbStatus;//前进后退状态 >0 前
    const short LR_STATUS_MAX_VALUE = 1;
    const short GB_STATUS_MAX_VALUE = 2;

public slots:
    void processPendingDatagrams();
    void timerUpdate();

signals:
    void picCapTureComplete(cv::Mat &frame);

private:
    #define MAX_PIC_INDEX 100 //图片索引最大值，不包含
    const int CAR_CTL_CMD_ID = 101;//控制车命令ID
    const int PKG_RESEND_CMD_ID = 102;//包重传命令ID

    QTimer *timer;
    int lastRecvTime,newRecvTime;
    int timerCnt;
    int transmitDelayTime;
    int randomOrderDelayTime;
    const unsigned short MAX_TIMER_CNT_VAL = 30000;//最大计数值

    QUdpSocket *udpSocket;
    QHostAddress clientIp;
    quint16 clientPort;

    #define PKG_MAX_LENGTH 60012
    char pkgData[PKG_MAX_LENGTH];

    static const int byteArrayNum = 400;
    int byteArrayPos;//待存放位置
    QByteArray byteArray[byteArrayNum];//存放收到到的小的图片数据包
    QByteArray udpData; //存储TCP接收数据

    #define NOT_RECV_DATA  0
    #define HAS_RECV_DATA  1
    #define RESEDN_REQUEST 2
    struct SinglePkgIndexInfoStr{
        unsigned short pkgPos; //pkg存放位置
        unsigned short pkgIndex;//包索引,未用
        unsigned short time; //接收到数据或请求重发的时间
        unsigned short sta; //0:未接收到数据 1：已接收到数据 2：请求重发
    };
    struct PhotoPkgInfoStr{
        vector<struct SinglePkgIndexInfoStr> sIndex;
        vector<unsigned char> data;
        int picIndex;//图片包索引
        int cnt;//已接收数据量
        int totoalNum;//压缩图片数据量
        int maxPkgCnt;//接收的最大包索引,重发用
    };

    static const int photoInfoNum = 4;
    struct PhotoPkgInfoStr photoInfo[photoInfoNum];
    int lastPhotoPos;

    const unsigned int PACKET_HEAD_LENGTH = 6;//包头长度
    short lastPkgPicIndex; //上一个数据包picIndex;
    short samePixIndexCnt; //相同picIndex计数;
    short picRecvIndex;//当前正在接收的图片索引
    short picShowIndex;//当前正在显示的图片索引

    //数据包头结构
    struct SingleResendInfoStr{
        unsigned short picIndex;//0~99图片索引
        unsigned short pkgIndex;//0～255数据包索引
        unsigned int size;//一幅图片数据量
    };
    vector<struct SingleResendInfoStr> resendInfo;//申请重发信息，包头结构

    int pkgCnt;
    QByteArray pkgDataGram;
    int key,cnt;
    int k,recvCnt;
    bool pkgSta;
    vector<unsigned char> dataEncode;

    int getDiff(int last,int now,int max);
    int findNextArrayPos(void);
    void checkShow(void);
    void pkgResendRequest(void);
};


#endif

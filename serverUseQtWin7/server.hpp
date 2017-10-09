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

#include <QUdpSocket>
#include <QWidget>

using namespace cv;
using namespace std;

class VideoTSServer : public QObject
{

    Q_OBJECT
public:
    VideoTSServer(QObject *parent = 0);
    ~VideoTSServer();

    void makeSocket(quint16 port);

    #define SEND_PKG_NUM_PER_PICTURE 4
    #define PIC_WIDRH 640
    #define PIC_HIGHT 480

    char server_ip[16]; //stream sent android camera
    int server_port;
    cv::Mat frame;//picture frame
    int frameCnt;

public slots:
    void processPendingDatagrams();
signals:
    void picCapTureComplete(cv::Mat &frame);

private:
    QUdpSocket *udpSocket;

    #define PKG_MAX_LENGTH 60012
    char pkgData[PKG_MAX_LENGTH];

    const unsigned int PACKET_HEAD_LENGTH = 6;

    int pkgCnt;
    QByteArray pkgDataGram;
    int key,cnt;
    int k,recvCnt;
    bool pkgSta;
    vector<unsigned char> dataEncode;
    int lastPkgIndex;

private:
    struct PkgHeadStr{
        int index; //0~255
        int sum; //0~65535
        int size;//0~16777215
    };
    struct PkgHeadStr pkgHead;

    void getPkgHead(struct PkgHeadStr &pkgHead);
};


#endif

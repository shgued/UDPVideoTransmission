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
#include <QtNetwork>
#include <QTcpSocket>
#include <QByteArray>

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
    cv::Mat frame;//picture frame，解压缩后的图片数据
    int frameCnt;//接收到的图片数量

public slots:
    void processPendingDatagrams();
    void sendFortune();

signals:
    void picCapTureComplete(cv::Mat &frame);

private:
    QTcpServer *tcpServer; //监听的TCPsocket
    QTcpSocket *clientConnection;//与客户端连接的socket
    int pkgCnt; //
    int key,cnt;
    int k,recvCnt;
    bool pkgSta;
    vector<unsigned char> dataEncode;//压缩的图片数据

    struct PkgHeadStr{
        short index; //小数据包索引
        short photoCnt;//图片索引
        int totalNum;//压缩图片数据量
    };

    struct PkgHeadStr pkgHead;
    static const int byteArrayNum = 500;
    int byteArrayPos;//待存放位置
    QByteArray byteArray[byteArrayNum];//存放解析到的小的图片数据包
    QByteArray tcpData; //存储TCP接收数据

    struct PhotoPkgInfoStr{
        vector<pair<short,short>> sIndex;//first：  second：
        int photoCnt;//图片包索引
        int cnt;//已接收数据量
        int totoalNum;//压缩图片数据量
    };

    static const int photoInfoNum = 8;
    int lastPhotoPos;
    struct PhotoPkgInfoStr photoInfo[photoInfoNum];

    void getPkgHead(struct PkgHeadStr &pkgHead,unsigned char *);
    void checkShow(void);
    int findNextArrayPos(void);
};


#endif

#ifndef __client_hpp_
#define __client_hpp_

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
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#include <vector>
#include <pthread.h>
#include <mutex>

class VideoTSClient{
public:
    //图片
    struct PictureStr{
        std::vector<unsigned char> pic;//图片
        unsigned int picIndex;//图片索引
        unsigned int size;//图片数据量
    };

    unsigned char picIndex;//正在发送的图片编号
    int resendPos;//resendPicBuf 位置索引
    static const int PIC_RESEND_BKP_NUM = 6;
    struct PictureStr resendPicBuf[PIC_RESEND_BKP_NUM];

    int sockfd;
    static const unsigned int addrLen = sizeof(struct sockaddr_in);

    VideoTSClient(pthread_mutex_t *mutex);
    ~VideoTSClient();
    int makeSocket(char * ip, int port);
    void sendFramToserver();
    void enableVideoShow();

private:
    pthread_mutex_t *sendMutex;//udp 发送互斥锁
    struct sockaddr_in addr;
    cv::Mat frame;//picture frame
    int pkgCnt;
    bool showVideoSta;//opencv 组件显示视频使能
};

#endif

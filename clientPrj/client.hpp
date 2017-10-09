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

using namespace cv;
class VideoTSClient{
private:
    int sockfd;
    struct sockaddr_in addr;
    cv::Mat frame;//picture frame
    int pkgCnt;
    static const unsigned int addrLen = sizeof(struct sockaddr_in);
    bool showVideoSta;
public:
    VideoTSClient();
    ~VideoTSClient();
    int makeSocket(char * ip, int port);
    void sendFramToserver();
    void enableVideoShow();
};

#endif

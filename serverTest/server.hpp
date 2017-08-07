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
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ifaddrs.h>

using namespace cv;
class VideoTSServer{
private:
    int sockfd;
    struct sockaddr_in addr;
    cv::Mat frame;//picture frame
    int pkgCnt;
    unsigned int addrLen;

public:
    VideoTSServer();
    ~VideoTSServer();

    int makeSocket(int port);
    void recvFrameFromClient();
};


#endif

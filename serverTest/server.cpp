//============================================================================
// Name        : Server.cpp
// Author      : shgued
// Version     : 1.0
// Copyright   : open source
// Description : OpenCV experimentation in C++, transform video with UDP
//============================================================================
#include "server.hpp"

#define TAG "Server- "

using namespace std;
using namespace cv;

#define SEND_PKG_NUM_PER_PICTURE 4
//#define PIC_WIDRH 640
//#define PIC_HIGHT 480
//#define IMG_PKG_SIZE (PIC_WIDRH*PIC_HIGHT*3/30)

struct PkgHeadStr{
    int index; //0~255
    int sum; //0~65535
    int size;//0~16777215
};

//struct ImgDataStr{
//    int pkgIndex;
//    int dataSize;
//    int sum;
//    unsigned char img[IMG_PKG_SIZE];
//};

#define PKG_MAX_LENGTH 60012
unsigned char pkgData[PKG_MAX_LENGTH];
struct PkgHeadStr pkgHead;

void setPkgHead(int index,int sum, int dataSize){
    pkgData[0] = index&0xf;
    pkgData[1] = sum%256;
    pkgData[2] = sum/256;
    pkgData[3] = dataSize%256;
    dataSize >>= 8;
    pkgData[4] = dataSize%256;
    dataSize >>= 8;
    pkgData[5] = dataSize%256;
}

void getPkgHead(struct PkgHeadStr &pkgHead){
    pkgHead.index = pkgData[0];
    pkgHead.sum = pkgData[1]|(pkgData[2]<<8);
    pkgHead.size = pkgData[3]|(pkgData[4]<<8)|(pkgData[5]<<8);
}


VideoTSServer::VideoTSServer(){
    addrLen = sizeof(struct sockaddr_in);
}
VideoTSServer::~VideoTSServer(){
}

int VideoTSServer::makeSocket(int port){
    int optVal;
    unsigned int optLen;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset((void*)&addr,0,addrLen);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    //缓冲区
    optVal=65536;
    optLen=sizeof(optVal);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void*)&optVal, optLen);
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optVal,&optLen);
    cout << "recv buf size:" << optVal << endl;
    if(bind(sockfd,(struct sockaddr*)(&addr),addrLen)<0){
        cout<<TAG<<"bind error!"<<endl;
        exit(1);
    }
    else cout<<TAG<<"bind succesfully!"<<endl;

    cout<<TAG<<"Opened socket port:"<< port <<"socket id:"<<sockfd<<endl;

    return sockfd;
}
void VideoTSServer::recvFrameFromClient(){
    int key,cnt;
    int i,k,sum,recvCnt = 0;
    bool pkgSta = true;
    vector<uchar> dataEncode;
    int lastPkgIndex = SEND_PKG_NUM_PER_PICTURE-1;

    Mat frame;//(PIC_HIGHT,PIC_WIDRH,CV_8UC3);//set by pivture size

    namedWindow("CameraCapture",1);

    for(;;)
    {
        cnt = recvfrom(sockfd, (void*)&pkgData, \
              sizeof(pkgData), 0, (struct sockaddr*)&addr,&addrLen);
        //cout << "addr:" << htonl(addr.sin_addr.s_addr)<< "port" << addr.sin_port<<endl;
        if(recvCnt%(SEND_PKG_NUM_PER_PICTURE*100) == 0)
            cout << "recvCnt:" << recvCnt << endl;
        recvCnt++;

        getPkgHead(pkgHead);
        if(cnt == pkgHead.size){
            //cout << "recIndex:" << pkgHead.index << endl;

            k = pkgHead.size;
            for(i=6,sum=0;i<k;i++){
                dataEncode.push_back(pkgData[i]);
                sum += pkgData[i];
            }
            if(sum%65536 != pkgHead.sum) cout << "pkg sum error" << sum <<endl;
            k = pkgHead.index - 1;
            if(k < 0) k = SEND_PKG_NUM_PER_PICTURE - 1;
            if(k != lastPkgIndex) pkgSta = false;
            lastPkgIndex = pkgHead.index;

            if(pkgHead.index == (SEND_PKG_NUM_PER_PICTURE-1)){
                if(pkgSta){
                    frame = imdecode(dataEncode, CV_LOAD_IMAGE_COLOR);
                    //cout << "image show" << endl;
                    imshow("CameraCapture", frame);
                }
                key = waitKey(1);
                if(key == 'q' || key == 'Q') exit(0);
                dataEncode.clear();
                pkgSta = true;
            }
        }
        else
            cout<<"pkg size error! cnt:"<< cnt << pkgHead.size <<endl;
    }
}

void showIPAdress(void){
    struct ifaddrs * ifAddrStruct=NULL;
        void * tmpAddrPtr=NULL;

        getifaddrs(&ifAddrStruct);

        printf("IP Adreess:\n");
        while (ifAddrStruct!=NULL) {
            if (ifAddrStruct->ifa_addr->sa_family == AF_INET) { // check it is IP4
                // is a valid IP4 Address
                tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            }
            else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
                // is a valid IP6 Address
                tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
                char addressBuffer[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            }
            ifAddrStruct=ifAddrStruct->ifa_next;
        }
}

char server_ip[16] = "192.168.199.244"; //stream sent android camera
int server_port = 5136;

int main(int argc,char **argv){
    VideoTSServer vs;
    const char *help1 = "-h";
    const char *help2 = "--help";
    int i,base;

    if(argc < 2){
        cout << "you hadn't input server port,use default!"<<endl;
        cout << "default: server port:"<<server_port<<endl;

        sleep(4);

    }
    else if(argc == 2){
        if(strcmp(argv[1],help1)==0 || strcmp(argv[1],help2)==0){
            cout << "cammand format1: ./server" << endl;
            cout << "cammand format2: ./server 5136" << endl;
            showIPAdress();
            exit(0);
        }
        else{
            i=0;
            while(argv[1][i] != '\0' && i<6) i++;
            if(i == 6) cout << "input server port error1!";
            if(i>0) i--;
            server_port = 0;
            base = 1;
            while(i>=0){
                server_port += base*(argv[1][i]-'0');
                base *= 10;
                i--;
            }
            if(server_port > 65535 || server_port <= 0)
                cout << "input server port error2";
        }
    }
    else
    {
        cout << "two much input param" << endl;
        return 3;
    }

    if(vs.makeSocket(server_port) < 0){
        cerr<<TAG<<"ERROR opening socket";
    }
    showIPAdress();
    vs.recvFrameFromClient();

    return 0;
}



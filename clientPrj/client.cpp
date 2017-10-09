//============================================================================
// Name        : client.cpp
// Author      : shgued
// Version     : 1.2
// Copyright   : open source
// Description : OpenCV experimentation in C++, transform video with UDP
// !! 标记重点关注
// 整理程序加入注释，将BGR格式图片转为RGB格式图片,发送数据包间加入延时
//============================================================================

#include "client.hpp"
#include <time.h>
#include <sys/time.h>

#define TAG "Client- "

using namespace std;
using namespace cv;

//每张图片分的数据包数量 ，客户端和服务器端要一致 !!
#define SEND_PKG_NUM_PER_PICTURE 4

//包头数据字节数
#define PACKET_HEAD_LENGTH 6

//数据包头结构
struct PkgHeadStr{
    int index; //0~255
    int sum; //0~65535
    int size;//0~16777215
};

//发送数据暂存
#define PKG_MAX_LENGTH 60012
unsigned char pkgData[PKG_MAX_LENGTH];
struct PkgHeadStr pkgHead;

VideoTSClient::VideoTSClient(){
    showVideoSta = false;
}
VideoTSClient::~VideoTSClient(){
}

//设置包头 index:包索引 sum：校验和 datasize：发送的数据量
void setPkgHead(int index,int sum, int dataSize){
    pkgData[0] = index&0xf;

    pkgData[1] = sum%256;
    pkgData[2] = sum/256;

    pkgData[3] = dataSize&0xff;
    dataSize >>= 8;
    pkgData[4] = dataSize&0xff;
    dataSize >>= 8;
    pkgData[5] = dataSize&0xff;
}
//获取包头
void getPkgHead(struct PkgHeadStr &pkgHead){
    pkgHead.index = pkgData[0];
    pkgHead.sum = pkgData[1]|(pkgData[2]<<8);
    pkgHead.size = pkgData[3]|(pkgData[4]<<8)|(pkgData[5]<<8);
}

//建立socket
int VideoTSClient::makeSocket(char * ip, int port){
    int optVal;
    unsigned int optLen;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

//    memset((void*)&addr,0,addrLen);
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr= htonl(INADDR_ANY);
//    addr.sin_port = htons(port);
//    if(bind(sockfd,(struct sockaddr*)(&addr),addrLen)<0){
//        cout<<TAG<<"bind error!"<<endl;
//        exit(1);
//    }
//    else cout<<TAG<<"bind succesfully!"<<endl;

    memset((void*)&addr,0,addrLen);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr=inet_addr(ip);
    addr.sin_port = htons(port);

    //发送缓冲区
    optVal=65536;
    optLen=sizeof(optVal);
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void*)&optVal, optLen);
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &optVal,&optLen);
    cout << "send buf size:" << optVal << endl;

    /*
    if(bind(sockfd,(struct sockaddr*)(&addr),addrLen)<0){
        cout<<TAG<<"bind error!"<<endl;
        exit(1);
    }
    else cout<<TAG<<"bind succesfully!"<<endl;*/

    cout << TAG << "Opened socket seccessfully!"<<endl;
    cout << "server ip:"<<ip<<" server port:"<<port<<" socket id:"<<sockfd<<endl;

    return sockfd;
}
//向服务器端即远程显示端发送图片信息，函数运行约占14～18ms
void VideoTSClient::sendFramToserver(){
    int key,pkgCnt,cnt;
    int i,k,sum,sendCnt = 0;;
    //struct ImgDataStr imgData;
    unsigned char *pImg,*pPkg;
    int picSize;

    time_t t;
    struct tm * lt;

    static int us;
    struct timeval tv;
    struct timezone tz;
    int us1,us2;

    VideoCapture cap(0);// open camera //第0个相机设备
    if(!cap.isOpened()) cout << "open cam faild!"<<endl;
    else cout << "open cam success!"<<endl;

    if(showVideoSta)
        namedWindow("videoClient",1);
    for(;;)
    {
        vector<uchar> dataEncode;

        gettimeofday (&tv , &tz); us1 = tv.tv_usec;
        cap >> frame; //采集图片占用时间有时可达0.1s以上

        gettimeofday (&tv , &tz); us2 = tv.tv_usec;
        us = us2 - us1; if(us < 0)  us += 1000000;

        //将pencv BGR格式传为RGB格式
        int  m,n;
        unsigned char *dat,ch;
        n =  frame.rows*frame.cols;
        dat = frame.data;
        if(0)//
        for(m=0;m<n;m++){
           ch = *dat;
           *dat = *(dat+2);
           *(dat+2) = *dat;
           dat += 3;
           //cout << "m1:" << m << " dat:" << (int)*dat << endl;
        }

        imencode(".jpg", frame, dataEncode);//7～8ms
        picSize = dataEncode.size();
        //cout << "after incode size:" << picSize << endl;

        /* //frame param
        cout<<"x"<<frame.cols<<endl;
        cout<<"y"<<frame.size().height<<endl;
        cout<<"depth"<<frame.depth()<<endl;
        cout<<"total"<<frame.total()<<endl;
        cout<<"elsmSize"<<frame.elemSize()<<endl;
        cout<<"type"<<frame.type()<<endl;
        cout<<"channels"<<frame.channels()<<endl;//l*/
        //return;

        //cvtColor(frame, edges, CV_BGR2GRAY);
        //GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
        //Canny(edges, edges, 0, 30, 3);
        //imshow("videoClient", edges);

        if(showVideoSta){ //-s opencv 组件显示图片
            frame = imdecode(dataEncode, CV_LOAD_IMAGE_COLOR);
            imshow("videoClient", frame);
            key = waitKey(30);
            //cout << "key:" << key <<endl;
            if(key == 'q' || key == 'Q') exit(0);
            continue;
        }                    

        time (&t);//获取Unix时间戳。
        lt = localtime (&t);//转为时间结构。
        if(sendCnt%100 == 0)
            printf ( "sendPicCnt:%d clock:%d time_s:%d scnt:%d smpus:%d\n",sendCnt,clock(),lt->tm_sec,cnt,us);//输出结果
        sendCnt++;

        k = picSize/SEND_PKG_NUM_PER_PICTURE;
        pImg = dataEncode.data();

        for(pkgCnt=0;pkgCnt<SEND_PKG_NUM_PER_PICTURE;pkgCnt++){
            if(pkgCnt == (SEND_PKG_NUM_PER_PICTURE-1)){
                k = picSize - (SEND_PKG_NUM_PER_PICTURE-1)*k;;
                if(k+PACKET_HEAD_LENGTH > PKG_MAX_LENGTH){
                    cout << "send data num > send buf size,error:6" << endl;
                    exit(6);
                }
            }
            //cout << "pkgcnt:" << pkgCnt << endl;//包索引
            pPkg = &pkgData[PACKET_HEAD_LENGTH];
            for(i=0,sum=0;i<k;i++,pImg++,pPkg++){
                *pPkg = *pImg;
                sum += *pImg;
            }
            setPkgHead(pkgCnt,sum,k+PACKET_HEAD_LENGTH);
            getPkgHead(pkgHead);
            //cout << "sum:" << pkgHead.sum << endl;
            //cout<<"pkgCnt"<<pkgHead.index<<"send:" << pkgHead.size<<endl;

            time (&t);//获取Unix时间戳。
            lt = localtime (&t);//转为时间结构。
            //printf ( "1:%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果

            clock_t ct1 = clock();

            //30～110us
            cnt = sendto(sockfd, (const void*)&pkgData, \
                  pkgHead.size, 0, (struct sockaddr*)&addr,addrLen);

            //为数据包发送预留时间，减少快速发送大量数据造成网络拥堵  !!
            //可以逐步增大时间到不出现或很少 error frame 为止
            usleep(5000);

            clock_t ct2 = clock();
            //printf("ct2:%d\n",ct2-ct1);

            time (&t);//获取Unix时间戳。
            lt = localtime (&t);//转为时间结构。
            //printf ( "2:%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果

            if (cnt < 0)
            {
                cerr<<TAG<<"ERROR writing to udp socket:"<<cnt;
                return;
            }
        }
    }
}

//使能只用opencv组件显示图片流
void VideoTSClient::enableVideoShow(){
    showVideoSta = true;
}
//显示ip地址信息
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

//默认地址和端口
char server_ip[16] = "192.168.199.229"; //default server ip
int server_port = 5136; //default server port

int main(int argc,char **argv){
    VideoTSClient vc;
    const char *helpOpt1 = "-h";
    const char *helpOpt2 = "--help";
    const char *showVideoOpt1 = "-s";
    const char *showVideoOpt2 = "--showVideo";
    int i,base;
    char ch;

    if(argc < 2){
        cout << "you hadn't input server ip,u and server port,use default!"<<endl;
        cout << "default: server ip:"<<server_ip<<" server port:"<<server_port<<endl;
    }
    else if(argc == 2){
        if(strcmp(argv[1],helpOpt1)==0 || strcmp(argv[1],helpOpt2)==0){
            cout << "cammand format1: ./client" << endl;
            cout << "cammand format2: ./client 192.168.199.229 5136" << endl;
            cout << "cammand format3: ./client -s" << endl;
            cout << "cammand format4: ./client --showVideo" << endl;
            cout << "cammand format5: ./client -h" << endl;
            cout << "cammand format6: ./client --help" << endl;
            showIPAdress();
            exit(0);
        }
        else if(strcmp(argv[1],showVideoOpt1)==0 || strcmp(argv[1],showVideoOpt2)==0){
            vc.enableVideoShow();
        }
        else{
            cout << "you hadn't input server ip and server port,use default!"<<endl;
            cout << "default: server ip:"<<server_ip<<" server port:"<<server_port<<endl;
            sleep(4);
        }
    }
    else if(argc == 3){
        i=0;
        while(i<16){
            ch = argv[1][i];
            if((ch<'0'||ch>'9') && ch!='.'&&ch!='\0'){
                cout << "ip format error! input ip:" << argv[1] << endl;
                return 2;
            }
            else {
                server_ip[i] = ch;
                if(ch == '\0')
                    break;
            }
            i++;
        }
        if(i==16 && ch!=0){
            cout << "ip format error! input ip:" << argv[1] << endl;
            return 2;
        }
        i=0;
        while(argv[2][i] != '\0' && i<6) i++;
        if(i == 6) {
            cout << "input server port error1!" << endl;
            return 3;
        }
        if(i>0) i--;
        server_port = 0;
        base = 1;
        while(i>=0){
            server_port += base*(argv[2][i]-'0');
            base *= 10;
            i--;
        }
        if(server_port > 65535 || server_port <= 0){
            cout << "input server port error2" << endl;
            return 4;
        }
    }
    if(vc.makeSocket(server_ip,server_port) < 0){
        cerr<<TAG<<"ERROR opening socket";
    }

    vc.sendFramToserver();

    return 0;
}



//============================================================================
// Name        : client.cpp
// Author      : shgued
// Version     : 2.1
// Copyright   : open source
// Description : OpenCV experimentation in C++, transform video with UDP
// !! 标记重点关注
// 修正线程函数中recvfrom返回-1的bug
//============================================================================

#include "client.hpp"
#include <time.h>
#include <sys/time.h>

#define TAG "Client- "
#define  PKG_RANDOM_ORDER_TEST  //测试数据包排序功能，定义则测试  交换 pkgIndex 0和1的发包顺序

using namespace std;
using namespace cv;

//---------客户端服务器端设置要一致
#define PACKAGE_HEAD_LENGTH 6 //包头长度
#define MAX_PACKAGE_SIZE (1460*2)//最大数据包长度
#define MAX_PACKAGE_DATA_NUM (MAX_PACKAGE_SIZE-PACKAGE_HEAD_LENGTH) //除去包头最大数据量

#define MAX_PIC_INDEX 100 //图片索引最大值，不包含
//---------客户端服务器端设置要一致

//数据包头结构
struct PkgHeadStr{
    unsigned char picIndex;//0~99图片索引
    unsigned char pkgIndex;//0～255数据包索引
    unsigned int size;//一幅图片数据量
};


struct PkgHeadStr pkgHead;//包头结构
pthread_mutex_t sendMutex;//UDP发送数据互斥锁

//发送数据暂存
#define PKG_MAX_LENGTH 60012
unsigned char pkgData[PKG_MAX_LENGTH];

VideoTSClient::VideoTSClient(pthread_mutex_t *mutex){
    int i,j;
    sendMutex = mutex;
    showVideoSta = false;
    for(i=0;i<PIC_RESEND_BKP_NUM;i++){
        resendPicBuf[i].size = 0;
        resendPicBuf[i].picIndex = MAX_PIC_INDEX;
    }
    resendPos = PIC_RESEND_BKP_NUM-1;
    picIndex = MAX_PIC_INDEX;
}
VideoTSClient::~VideoTSClient(){
}

//设置包头 index:包索引 sum：校验和 datasize：发送的数据量
void setPkgHead(unsigned char picIndex,unsigned char pkgIndex, unsigned int dataSize){
    pkgData[0] = picIndex;

    pkgData[1] = pkgIndex;

    pkgData[2] = dataSize&0xff;
    dataSize >>= 8;
    pkgData[3] = dataSize&0xff;
    dataSize >>= 8;
    pkgData[4] = dataSize&0xff;
    dataSize >>= 8;
    pkgData[5] = dataSize&0xff;
}

//解析包头
struct PkgHeadStr getPkgHead(unsigned char *pkgData){
    struct PkgHeadStr ph;
    ph.picIndex = pkgData[0];
    ph.pkgIndex = pkgData[1];
    ph.size = pkgData[2] | (pkgData[3]<<8) | (pkgData[4]<<16) | (pkgData[5]<<24);
    return ph;
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

//    if(bind(sockfd,(struct sockaddr*)(&addr),addrLen)<0){
//        cout<<TAG<<"bind error!"<<endl;
//        exit(1);
//    }

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
    int key,pkgCnt,cnt,sendNum,toSendDataNum,test;
    int i,k,sum,sendCnt = 0;;
    //struct ImgDataStr imgData;
    unsigned char *pImg,*pPkg;
    int picSize;

    time_t t;
    struct tm *lt;

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
        gettimeofday (&tv , &tz); us1 = tv.tv_usec;
        cap >> frame; //采集图片等待时间有时可达0.1s以上

        gettimeofday (&tv , &tz); us2 = tv.tv_usec;//获取us级时间
        us = us2 - us1; if(us < 0)  us += 1000000;

        resendPos++;        
        if(resendPos >= PIC_RESEND_BKP_NUM) resendPos = 0;
        //resendPicBuf[resendPos].picIndex = MAX_PIC_INDEX;
        imencode(".jpg", frame, resendPicBuf[resendPos].pic);//7～8ms

        picSize = resendPicBuf[resendPos].pic.size();
        resendPicBuf[resendPos].size = picSize;
        picIndex++;
        if(picIndex >= MAX_PIC_INDEX) picIndex = 0;
        resendPicBuf[resendPos].picIndex = picIndex;
        //cout << "after incode size:" << picSize << endl;

        /* //frame 参数
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

        //showVideoSta = true;
        if(showVideoSta){ //-s opencv 组件显示图片
            frame = imdecode(resendPicBuf[resendPos].pic, CV_LOAD_IMAGE_COLOR);
            imshow("videoClient", frame);
            key = waitKey(30);
            //cout << "key:" << key <<endl;
            if(key == 'q' || key == 'Q') exit(0);
            continue;
        }

//       frame = imdecode(resendPicBuf[resendPos].pic, CV_LOAD_IMAGE_COLOR);
//       imshow("videoClient", frame);waitKey(1);

        time (&t);//获取Unix时间戳。
        lt = localtime (&t);//转为时间结构。
        if(sendCnt%100 == 0)
            printf ( "sendPicCnt:%d clock:%d time_s:%d scnt:%d smpus:%d\n",sendCnt,clock(),lt->tm_sec,cnt,us);//输出结果
        sendCnt++;

        for(pkgCnt=0,sendNum=0;sendNum<picSize;pkgCnt++,sendNum += MAX_PACKAGE_DATA_NUM){
            if(sendNum + MAX_PACKAGE_DATA_NUM < picSize){
                toSendDataNum = MAX_PACKAGE_DATA_NUM;
            }
            else toSendDataNum = picSize - sendNum;

            //#define  PKG_RANDOM_ORDER_TEST  //乱序发包，测试数据包排序功能
            #ifdef PKG_RANDOM_ORDER_TEST
                        if(pkgCnt == 0) test = 1;
                        else if(pkgCnt == 1) test = 0;
                        else test = pkgCnt;
            #else
                        test = pkgCnt;
            #endif

            pthread_mutex_lock(sendMutex);//UDP发送 互斥
            pImg = resendPicBuf[resendPos].pic.data()+MAX_PACKAGE_DATA_NUM*test;
            pPkg = &pkgData[PACKAGE_HEAD_LENGTH];
            memcpy(pPkg,pImg,toSendDataNum);

            setPkgHead(resendPicBuf[resendPos].picIndex, test, resendPicBuf[resendPos].size);
            pkgHead = getPkgHead(pkgData);
            //cout<<"pkgCnt"<<(int)pkgHead.pkgIndex<<"send:" << pkgHead.size<<endl;

            time (&t);//获取Unix时间戳。
            lt = localtime (&t);//转为时间结构。
            //printf ( "1:%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果

            clock_t ct1 = clock();

            //30～110us
            cnt = sendto(sockfd, (const void*)&pkgData, \
                  toSendDataNum+PACKAGE_HEAD_LENGTH, 0, (struct sockaddr*)&addr,addrLen);

            pthread_mutex_unlock(sendMutex);

            //为数据包发送预留时间，减少快速发送大量数据造成网络拥堵
            //可以逐步增大时间到不出现或很少 error frame 为止
            if(sendCnt%5 == 4)
                usleep(2000);

            clock_t ct2 = clock();
            //printf("ct2:%d\n",ct2-ct1);

            time (&t);//获取Unix时间戳。
            lt = localtime (&t);//转为时间结构。
            //printf ( "2:%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果

            if (cnt < 0){
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
char server_ip[16] = "192.168.199.244"; //default server ip
int server_port = 5136; //default server port

int udpSockfd = -1;//多线程使用
short lrStatus;//左右转状态 >0 右
short gbStatus;//前进后退状态 >0 前
const short LR_STATUS_MAX_VALUE = 2;
const short GB_STATUS_MAX_VALUE = 2;
const int CAR_CTL_CMD_ID = 101;//控制车命令ID
const int PKG_RESEND_CMD_ID = 102;//包重传命令ID
int recvFps;//服务器端的每秒接收图片数量
int sendFps;//客户端端的每秒发送图片数量

VideoTSClient vc(&sendMutex);

//接收服务器端命令和重发包请求，重发数据包 静态函数
void * threadFun(void *sockfd){
    socklen_t len = sizeof(struct sockaddr);//不赋值（结构体大小），recvfrom可能返回-1，在ubuntu下不发生错误，开发板上出错了
    struct sockaddr addr;
    int cnt,num;
    static int t = 10;
    unsigned char dat[1600];

    while(1){
        if(udpSockfd != -1){            
            cnt = recvfrom(udpSockfd, (void*)&dat,  \
                          1600, 0, &addr,&len);
            //cout << "cnt:" << cnt << " udpSockfd:" <<  udpSockfd << endl;
            //获取错误信息
            if(cnt == -1) cout << "recvfrom error:" << strerror(errno) <<endl;
        }
        if(cnt>4 && dat[0]==0x78 && dat[1]==0x56 && dat[2]==0x34 && dat[3]==0x12){
            if(dat[4] == CAR_CTL_CMD_ID && cnt == 7){//车控命令长度
                gbStatus = dat[5];
                if(gbStatus > 128) gbStatus = -(256-gbStatus);
                lrStatus = dat[6];
                if(lrStatus > 128) lrStatus = -(256-lrStatus);
                //cout << "gb:" << gbStatus << " lr:" << lrStatus << endl;
            }
            else if(dat[4] == PKG_RESEND_CMD_ID){
                recvFps = dat[5];
                //cout << "recvFps:" << recvFps << "    cnt:" << cnt << endl;
                if((cnt-6)%6 == 0){
                    int i,j;
                    int pkgIndex,picIndex,size;
                    unsigned char * pPkg,*pImg;
                    int toSendNum;
                    num = (cnt-6)/6;
                    for(i=6;i<cnt;){
                        picIndex = dat[i++];
                        pkgIndex = dat[i++];
                        size = dat[i] | (dat[i+1] << 8) | (dat[i+2] << 16) | (dat[i+3] << 24);
                        i += 4;
                        //cout << "picIndex:" << picIndex << " pkgIndex:" << pkgIndex << " size:" << size << endl;
                        for(j=0;j<vc.PIC_RESEND_BKP_NUM;j++){
                            if(vc.resendPicBuf[j].picIndex == picIndex && vc.resendPicBuf[j].size == size){
                                pthread_mutex_lock(&sendMutex); //UDP发送访问互斥
                                pImg = vc.resendPicBuf[j].pic.data()+MAX_PACKAGE_DATA_NUM*pkgIndex;
                                pPkg = &pkgData[PACKAGE_HEAD_LENGTH];
                                if(MAX_PACKAGE_DATA_NUM*(pkgIndex+1) > size) toSendNum = size - MAX_PACKAGE_DATA_NUM*pkgIndex;
                                else toSendNum = MAX_PACKAGE_DATA_NUM;
                                memcpy(pPkg,pImg,toSendNum);

                                setPkgHead(vc.resendPicBuf[j].picIndex, pkgIndex, vc.resendPicBuf[j].size);

                                //30～110us
                                sendto(vc.sockfd, (const void*)&pkgData, \
                                      toSendNum+PACKAGE_HEAD_LENGTH, 0, (struct sockaddr*)&addr,vc.addrLen);
                                pthread_mutex_unlock(&sendMutex);
                                break;
                            }
                        }
                    }
                }
                else
                    cout << "resend request pkg error!" << endl;

            }
            else cout << "cnt:" << cnt  << " ID:"  << (int)pkgData[4] << endl;
        }
        else cout << "cnt:" << cnt << endl;

    }
    return NULL;
}

int main(int argc,char **argv){    
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

    udpSockfd = vc.sockfd;
    pthread_mutex_init(&sendMutex,NULL);

    pthread_t thread;
    pthread_create(&thread,NULL,threadFun,NULL);

    vc.sendFramToserver();

    return 0;
}



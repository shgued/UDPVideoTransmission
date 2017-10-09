//============================================================================
// Name        : client.cpp
// Author      : shgued
// Version     : 1.0
// Copyright   : open source
// Description : OpenCV experimentation in C++, transform video with UDP
//============================================================================
#include "client.hpp"

#define TAG "Client- "

using namespace std;
using namespace cv;

#define PACKAGE_HEAD_LENGTH 10 //包头
#define TCPSOCKET_PACKAGE_DATA_NUM (1448-PACKAGE_HEAD_LENGTH) //除去包头最大数据量
#define MAX_TCP_PACKAGE_SIZE (TCPSOCKET_PACKAGE_DATA_NUM + PACKAGE_HEAD_LENGTH)//最大数据包长度

struct PkgHeadStr{    
    unsigned short index;//小数据包索引
    unsigned short photoCnt;//图片索引
    unsigned int totalNum;//压缩图片数据量
    int syn;
};

#define PKG_MAX_LENGTH 60012
unsigned char pkgData[PKG_MAX_LENGTH];

struct PkgHeadStr pkgHead;


VideoTSClient::VideoTSClient(){
    showVideoSta = false;
}
VideoTSClient::~VideoTSClient(){
}

//设置包头
void setPkgHead(unsigned short index,unsigned int totalNum,int photoCnt,int len){
    pkgData[0] = index&0xff;
    index >>= 8;
    pkgData[1] = index&0x0ff;
    pkgData[2] = totalNum&0x0ff;
    totalNum >>= 8;
    pkgData[3] = totalNum&0x0ff;
    totalNum >>= 8;
    pkgData[4] = totalNum&0x0ff;
    totalNum >>= 8;
    pkgData[5] = photoCnt&0x0ff;
    pkgData[6] = 0xaa;
    pkgData[7] = 0xaa;
    pkgData[8] = len&0xff;
    len >>= 8;
    pkgData[9] = len&0xff;

}
//解析包头
struct PkgHeadStr getPkgHead(unsigned char *pkgData){
    struct PkgHeadStr ph;
    ph.index = pkgData[0] | (pkgData[1]<<8);
    ph.totalNum = pkgData[2] | (pkgData[3]<<8) | (pkgData[4]<<16);
    ph.photoCnt = pkgData[5];
    return ph;
}

//创建tcp socket
int VideoTSClient::makeSocket(char * ip, int port){
    int optVal;
    unsigned int optLen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    memset((void*)&addr,0,addrLen);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr=inet_addr(ip);
    addr.sin_port = htons(port);

    if(connect(sockfd,(struct sockaddr*)(&addr),sizeof(struct sockaddr))<0){
        cout << "socket connect errot!" << endl;
    }

    //发送缓冲区
    optVal=265536;
    optLen=sizeof(optVal);
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void*)&optVal, optLen);
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &optVal,&optLen);
    cout << "send buf size:" << optVal << endl;

    cout << TAG << "Opened socket seccessfully!"<<endl;
    cout << "server ip:"<<ip<<" server port:"<<port<<" socket id:"<<sockfd<<endl;

    return sockfd;
}

//向tcp server 发送图片数据
void VideoTSClient::sendFramToserver(){
    int key,pkgCnt,cnt;
    int i,sendCnt = 0;;
    //struct ImgDataStr imgData;
    unsigned char *pImg,*pPkg;
    int picSize;

    time_t t;
    struct tm * lt;

    VideoCapture cap(0);// open camera //第0个相机设备
    if(!cap.isOpened()) cout << "open cam faild!"<<endl;
    else cout << "open cam success!"<<endl;

    if(showVideoSta) //使用opencv 库显示图片流
        namedWindow("videoClient",1);
    for(;;)
    {
        vector<uchar> dataEncode;
        cap >> frame; // get a new frame from camera

//        if(frame.total() != (PIC_WIDRH*PIC_HIGHT)){
//            cout << "camera capture picture size error!" << endl;
//            cout << "PIC_WIDRH:" << PIC_WIDRH << "frame.cols:" << frame.cols << endl;
//            cout << "PIC_HIGHT" << PIC_HIGHT << "frame.rows:" << frame.rows << endl;
//            exit(6);
//        }

        imencode(".jpg", frame, dataEncode);
        picSize = dataEncode.size();        

        /* //frame param
        cout<<"x"<<frame1.cols<<endl;
        cout<<"y"<<frame1.size().height<<endl;
        cout<<"depth"<<frame1.depth()<<endl;
        cout<<"total"<<frame1.total()<<endl;
        cout<<"elsmSize"<<frame1.elemSize()<<endl;
        cout<<"type"<<frame1.type()<<endl;
        cout<<"channels"<<frame1.channels()<<endl;*/

        //cvtColor(frame, edges, CV_BGR2GRAY);
        //GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
        //Canny(edges, edges, 0, 30, 3);
        //imshow("videoClient", edges);

        //使用opencv 库显示图片流
        if(showVideoSta){
            frame = imdecode(dataEncode, CV_LOAD_IMAGE_COLOR);
            //cout << "wit:" << frame.cols << "  " << frame.rows << endl;
            imshow("videoClient", frame);
            key = waitKey(100);
            //cout << "key:" << key <<endl;
            if(key == 'q' || key == 'Q') exit(0);
            continue;
        }

        time (&t);//获取Unix时间戳。
        lt = localtime (&t);//转为时间结构。
        if(sendCnt%100 == 0)
            printf ( "sendCnt:%dclock:%d time_s:%d cnt:%d\n",sendCnt,clock(),lt->tm_sec,cnt);//输出结果
        sendCnt++;

        if(1){
            int pkgNum = picSize/TCPSOCKET_PACKAGE_DATA_NUM;
            int sendNum=0;
            if(picSize%TCPSOCKET_PACKAGE_DATA_NUM) pkgNum++;
            pImg = dataEncode.data();
            photoCnt++;
            photoCnt %= 128;
            for(pkgCnt=0;pkgCnt<pkgNum;pkgCnt++){
                if(pkgCnt == (pkgNum-1)){
                    sendNum = picSize - (pkgNum-1)*TCPSOCKET_PACKAGE_DATA_NUM;;
                }
                else sendNum = TCPSOCKET_PACKAGE_DATA_NUM;
                //cout << "pkgcnt:" << pkgCnt << endl;
                pPkg = &pkgData[PACKAGE_HEAD_LENGTH];
                for(i=0;i<sendNum;i++,pImg++,pPkg++){
                    *pPkg = *pImg;                    
                }
                setPkgHead(pkgCnt,picSize,photoCnt,sendNum+PACKAGE_HEAD_LENGTH);
                pkgHead = getPkgHead(pkgData);

//                //cout << "sum:" << pkgHead.sum << endl;
                //cout<<"pkgCnt1:"<<pkgHead.index<<"  send1:" << pkgHead.totalNum<<endl;
//                cout<<"pkgCnt2:"<<pkgCnt<<"  send2:" << picSize <<endl;
                time (&t);//获取Unix时间戳。
                lt = localtime (&t);//转为时间结构。
                //printf ( "1:%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果

                clock_t ct1 = clock();

                cnt = send(sockfd, (const void*)&pkgData, \
                      sendNum+PACKAGE_HEAD_LENGTH, 0);

                if(cnt != sendNum+PACKAGE_HEAD_LENGTH)
                    cout << "tcp send error, send:" <<  cnt << " real:" << sendNum+PACKAGE_HEAD_LENGTH << endl;

                //if(pkgCnt%10 == 9)  usleep(8000);

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
        else
            cout<<"camera capture picture size error!"<<endl;
    }
}

//使能使用opencv 库显示图片流
void VideoTSClient::enableVideoShow(){
    showVideoSta = true;
}

//获取本机ip地址信息
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


char server_ip[16] = "192.168.199.229"; //default server ip
int server_port = 39078; //default server port

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
            sleep(3);
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



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


VideoTSClient::VideoTSClient(){
    showVideoSta = false;
}
VideoTSClient::~VideoTSClient(){
}

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
void VideoTSClient::sendFramToserver(){
    int key,pkgCnt,cnt;
    int i,k,sum,sendCnt = 0;;
    //struct ImgDataStr imgData;
    unsigned char *pImg,*pPkg;
    int picSize;

    time_t t;
    struct tm * lt;

    VideoCapture cap(0);// open camera //第0个相机设备
    if(!cap.isOpened()) cout << "open cam faild!"<<endl;
    else cout << "open cam success!"<<endl;

    if(showVideoSta)
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
        //cout << "after incode size:" << picSize << endl;

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

        if(showVideoSta){
            frame = imdecode(dataEncode, CV_LOAD_IMAGE_COLOR);
            imshow("videoClient", frame);
            waitKey(30);
        }

        key = waitKey(1);//ms
        //cout << "key:" << key <<endl;
        if(key == 'q' || key == 'Q') exit(0);

        if(showVideoSta)
            continue;

        time (&t);//获取Unix时间戳。
        lt = localtime (&t);//转为时间结构。
        if(sendCnt%100 == 0)
            printf ( "sendCnt:%dclock:%d time_s:%d cnt:%d\n",sendCnt,clock(),lt->tm_sec,cnt);//输出结果
        sendCnt++;

        if(1){
            k = picSize/SEND_PKG_NUM_PER_PICTURE;
            pImg = dataEncode.data();
            for(pkgCnt=0;pkgCnt<SEND_PKG_NUM_PER_PICTURE;pkgCnt++){
                if(pkgCnt == (SEND_PKG_NUM_PER_PICTURE-1)){
                    k = picSize - (SEND_PKG_NUM_PER_PICTURE-1)*k;;
                }
                //cout << "pkgcnt:" << pkgCnt << endl;
                pPkg = &pkgData[6];
                for(i=0,sum=0;i<k;i++,pImg++,pPkg++){
                    *pPkg = *pImg;
                    sum += *pImg;
                }
                setPkgHead(pkgCnt,sum,k+6);
                getPkgHead(pkgHead);
                //cout << "sum:" << pkgHead.sum << endl;
                //cout<<"pkgCnt"<<pkgHead.index<<"send:" << pkgHead.size<<endl;

                time (&t);//获取Unix时间戳。
                lt = localtime (&t);//转为时间结构。
                //printf ( "1:%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果

                clock_t ct1 = clock();
                cnt = sendto(sockfd, (const void*)&pkgData, \
                      pkgHead.size, 0, (struct sockaddr*)&addr,addrLen);
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
void VideoTSClient::enableVideoShow(){
    showVideoSta = true;
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


char server_ip[16] = "192.168.199.244"; //default server ip
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



# UDP局域网视频传输 V2.1

使用环境需求
：
客户端：负责视频采集和传输

Ubuntu 16.04 

OpenCV v3.2

qt5.7.0 （可选，作为开发环境）

g++ v4.9.2
make v4.0 

服务器端：负责视频接收和显示

Ubuntu 16.04 
OpenCV v3.2
qt5.7.0 （QT版必选）

g++ v4.9.2
make v4.0 



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


版本更改：
V1.0 第一版
V1.1 整理程序
V1.2 整理V1.1程序，并添加注释，将BGR格式图片转为RGB格式图片,发送数据包间加入延时，
增加TCP通信传输图片流作为对比。
V2.0 udp通信方式增加了接受包排序和丢包重发功能


V2.1 修复UDP2.0客户端程序移植到开发板上recvfrom返回-1的问题，修改read么文档，增加UDP2.0
服务器端示例图片

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


工作原理简述:
  
  
udp1.2版：客户端打开摄像头,使用opencv3.2库抽取图片帧,将图片帧压缩为jpg格式图片,通过
udpsocket发送压缩后的图片到
服务器端,每帧图片分四个数据包发送，服务器端对接收到的压缩
图片解压缩，显示图片流。因为传送的数据是压缩的图片,接收数据
包乱序或丢包将放弃该帧图片
的显示。
  
  
tcp1.0版：客户端打开摄像头,使用opencv3.2库抽取图片帧,将图片帧压缩为jpg格式图片,将压缩
图片分为若干个大小为1488字
节的小包，对小包加入包头，最后一个数据包可能少于1488字节，将
这些小数据包依次发送给服务器端。因为TCP发送的是数据流，因
此对小包加入了包头用于数据s麻
烦，而且包头也要作为数据发送，发送数据量更大，服务器端和客户端秩序建立一次连接，socktet
信
息是固定的，方便双向通信。也可以每发送一幅图片建立一次连接，用重新建立连接来同步信息，
这样socket信息总在变，双向通信有
限制，建立连接也需要时间。服务器端对接收到的压缩图片解
压缩，显示图片流。由于tcp有发送应答机制，应答时间是消息在网络中传
一个往返所需时间，一幅
图片可能需要多次应答，输图片流显示延迟比较大。TCP具有包排序，包重发，流量控制等功能，也
是它的优点。

    
udp2.0版：客户端打开摄像头,使用opencv3.2库抽取图片帧,将图片帧压缩为jpg格式图片,将压缩图
片分为若干个大小为2920字
节的小包，对小包加入包头，最后一个数据包可能少于2920字节，将这些
小数据包依次发送给服务器端。客户端定义了压缩图片的缓冲区，
服务器端对接收到的数据包包头信
息将数据包数据放到服务器端接收图片缓冲区的相应位置，这一过程完成了对数据包的排序，每接
收到一
个数据包都会根据transmitDelayTime和randomOrderDelayTime两个时间阀值检查当前正在接收
和上一张图片是否有丢失的数据包，如果
有则产生重发请求信息，在1ms定时器中发送重发数据包的请
求。每接收到一个数据包后后会检查是接收到了一幅完整的图片，如果接收到
了则对接收到的压缩图片
解压缩，并发送显示图片的信号，触发图片的更新显示。


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ubantu qt 安装：

1.到官网下载qt-opensource-linux-x64-android-5.7.0.run安装包

2.安装命令：

cd .run 文件路径

sudo chmod 755 qt-opensource-linux-x64-android-5.7.0.run

sudo ./qt-opensource-linux-x64-android-5.7.0.run 
显示图形安装界面
下一步，要联网，填写在官网注册的账户，未注册可以使用 用户名：shgued@126.com 
密码：shgued@126.com
将qt安装在新建的文件夹中（卸载时该文件夹中所有问件会被删除）
等待安装完毕

3.编译问题：/usr/local/Qt5.7.0/5.7/gcc_64/include/QtGui/qopengl.h:129: error: GL/gl.h: No such 
file or directory

安装依赖： sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev

ubantu 
opencv3.2安装： (转载自)http://blog.csdn.net/lzh2912/article/details/52495039 

1.下载安装包opencv-3.2.0.zip,也可以自行上OpenCV官网下载http://opencv.org/。解压安装包到你想要
的地方，本文以存放主文件夹下。

2.可以用以下代码进行安装
    
基本：sudo apt-get install build-essential
    
必须：sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
    
可选：sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev 
libtiff-dev libjasper-dev libdc1394-22-dev

3. 利用终端进入opencv3文件夹，代码如下：

cd ~/opencv-3.2.0
4. 
新建build文件夹存放opencv的编译文件，进入build文件夹代码如下

mkdir release
cd release

5. 配置opencv文件，代码如下：

cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local ..

提示:cmake 过程中要下载文件，网络环境不好，连接手机热点可能把文件下载下来（费流量，只下载wifi无法下载的文件）

6. 进行编译，代码如下：

make

7. 安装opencv库到系统，代码如下：

sudo make install

sudo ldconfig  #不用重启就能找到opencv头文件
这样OpenCV就可以使用了。


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

开发说明：

使用QT作为开发环境：

1.在qt下打开.pro文件工程文件，点击配置，修改构建目录到 工程所在文件夹/obj/debug

2.修改pro文件设置opencv3头文件和库文件路径,即修改

#header file path
INCLUDEPATH += -I$$"/usr/local/include/opencv2"

#library path
LIBS += -L$$"/usr/local/lib/"

3.开发编译使用


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


使用说明:

客户端和服务器端需在同一局域网下,即连接同一路由器,或在同一主机下测试,对于发送的数据包结构服务器端和客户端要一致.


设置使用的摄像头
client.cpp:VideoCapture cap(0);// open camera //第0个相机设备
客户端：

1.切换到clientPrj目录

2.make clean

3.make

4. ./client -h 查看命令格式

5.输入命令开启客户端

命令示例:

./client
you hadn't input server ip,u and server port,use default!

default: server ip:192.168.199.229 server port:5136

send buf size:131072
Client- Opened socket seccessfully! (正常打开socket)

server ip:192.168.199.229 server port:5136 socket id:10

open cam success! (表示识别了摄像头)

sendCnt:0clock:38872 time_s:59 cnt:0


服务器端:

1.切换到serverTest目录

2.make clean

3.make
4. ./server -h 查看命令格式

5.输入命令开启客户端

./server

命令示例:

you hadn't input server port,use default!
default: server port:5136

recv buf size:131072

Server- bind succesfully!
Server- Opened socket port:5136socket id:3

IP Adreess:
lo IP Address 127.0.0.1

wlp3s0 IP Address 192.168.199.244 (本机ip)

lo IP Address ::

wlp3s0 IP Address 0:0:fe80::c84:bc26

GLib-GIO-Message: Using the 'memory' GSettings backend.  Your settings will not be saved or shared with other applications.

recvCnt:0

recvCnt:800


使用QT版:
qtcreator 中打开工程编译执行,设置端口号,点击open开始接收.

注：tcp版本的端口号是不能自行设置的，需要点击服务器端（打开按钮），获取到随机分配的端口号，再将客户端与服务器端相连接。
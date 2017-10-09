#include "mainwindow.h"
#include <QApplication>
#include <QTimer>

#include "server.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QTimer tim;
    tim.start(1000);

    QObject::connect( &tim,SIGNAL(timeout()), &w, SLOT(showFPSSlot()) );

//    VideoTSServer vs;
//    vs.makeSocket(5136);

    return a.exec();
}

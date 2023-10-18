#include <QCoreApplication>
#include "sharedio/sharedio.hpp"
#include <QTimer>
#include <QDebug>
#include "../id.h"
#include <signal.h>

void setShutDownSignal( int signalId );
void handleShutDownSignal( int signalId );

int main(int argc, char *argv[])
{
    setShutDownSignal( SIGINT ); // shut down on ctrl-c
    setShutDownSignal( SIGTERM ); // shut down on killall

    QCoreApplication a(argc, argv);

    auto sharedIO = new SharedIO(SAI_APP_2, &a);

    if (!sharedIO->open("test_sharedio1", 64))
        qDebug() << "open error";

    QObject::connect(sharedIO, &SharedIO::readyRead,
                     [&](){
                         auto result = sharedIO->read();
                         qDebug() << result.second << QString(result.first);
                     });

    return a.exec();
}



void setShutDownSignal( int signalId )
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &handleShutDownSignal;
    if (sigaction(signalId, &sa, NULL) == -1)
    {
        qDebug() << "setting up termination signal";
        exit(1);
    }
}

void handleShutDownSignal( int )
{
    qDebug() << "shutdown...";
    QCoreApplication::exit(0);
}

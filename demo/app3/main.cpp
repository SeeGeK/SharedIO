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

    auto sharedIO = new SharedIO(SAI_APP_3, &a);

    if (!sharedIO->open("test_sharedio1", 64))
        qDebug() << "open error";

    QObject::connect(sharedIO, &SharedIO::errorOccurred, [&](const QString &text){ qDebug() << text; });

    QTimer::singleShot(2000, [&](){
        sharedIO->write("Message for all", SAI_APP_ALL);
        sharedIO->write("Message for 1", SAI_APP_1);
        sharedIO->write("Message for 2", SAI_APP_2);
        sharedIO->write("Message for self", SAI_APP_3);

        sharedIO->setMaxWriteWait(3000);

        for (int i = 0; i < 10; i++)
        {
            if (sharedIO->write(QString("Message %1").arg(i).toUtf8(), SAI_APP_ALL) == 0)
                break;
        }
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

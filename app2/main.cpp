#include <QCoreApplication>
#include "../sharedio/sharedio.hpp"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    SharedIO sharedIO;
    sharedIO.setId(2);
    if (!sharedIO.open("test_sharedio", 32))
        qDebug() << "open error";

    sharedIO.setCheckInterval(1);

    QObject::connect(&sharedIO, &SharedIO::readyRead,
                     [&sharedIO](){
                         auto result = sharedIO.readAll();
//                         qDebug() << QString(result);
                         sharedIO.write(QString("121345").toUtf8(), 1);
                     });

    sharedIO.write(QString("121345").toUtf8(), 1);

    return a.exec();
}

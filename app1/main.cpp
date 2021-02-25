#include <QCoreApplication>
#include "../sharedio/sharedio.hpp"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    SharedIO sharedIO;
    sharedIO.setId(1);
    if (!sharedIO.open("test_sharedio", 64))
        qDebug() << "open error";

    sharedIO.setCheckInterval(1);

    QObject::connect(&sharedIO, &SharedIO::readyRead,
                     [&sharedIO](){
                         auto result = sharedIO.readAll();
//                         qDebug() << QString(result);
                         sharedIO.write(QString("absdef").toUtf8(), 2);
                     });

    return a.exec();
}

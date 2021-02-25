#ifndef SHAREDIO_HPP
#define SHAREDIO_HPP

#include <QSharedMemory>

class QTimer;

class SharedIO : public QSharedMemory
{
    Q_OBJECT
public:
    SharedIO(QObject *parent = nullptr);

    static constexpr quint32 SHARED_ID = quint32(-1);

    QByteArray readAll();
    void write(const QByteArray &writeData, quint32 destinationID = SHARED_ID);

    bool open(const QString &key, qint32 size = 4096, AccessMode mode = AccessMode::ReadWrite);

    quint32 id() const;
    void setId(const quint32 &id);
    void setCheckInterval(qint32 msec);

    quint32 getSenderId() const;
    quint32 getDestinationId() const;

signals:
    void readyRead();
    void pause();
    void start();

private:

    struct State
    {
        quint32 senderId;
        quint32 destinationId;
        qint32 bytesWrited;
    };
    struct Info
    {
        quint32 creatorID;
        qint32 createSize;
    };


    quint32 _id = SHARED_ID;
    qint32 _size;
    QTimer *_timer = nullptr;

    bool _paused = false;

    qint32 _checkInterval_msec = 10;

    void checkState();

    State getState() const;
    void setState(const State &state);

    Info getInfo() const;
    void setInfo(const Info &state);

    static constexpr quint32 STATE_POSITION = 0;
    static constexpr quint32 INFO_POSITION = STATE_POSITION + sizeof(State);
    static constexpr quint32 DATA_POSITION = INFO_POSITION + sizeof(Info);
};

#endif // SHAREDIO_HPP

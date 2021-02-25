#include "sharedio.hpp"
#include <QDebug>
#include <QTimer>
#include <QTime>

SharedIO::SharedIO(QObject *parent) :
    QSharedMemory(parent)
{
    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &SharedIO::checkState);
    connect(this, &SharedIO::pause, this, [this](){
        _timer->stop();
        _paused = true;
    });
    connect(this, &SharedIO::pause, this, [this](){
        _timer->start();
        _paused = false;
    });

}

QByteArray SharedIO::readAll()
{
    QByteArray readData;

    lock();

    const char *from = static_cast<const char*>(constData());
    if (from) {
        readData.resize(_size);
        State state = getState();
        state.senderId = SHARED_ID;
        memcpy(readData.data(), &from[sizeof(State)], _size);
        setState(state);
    }

    unlock();

    return readData;
}

void SharedIO::write(const QByteArray &writeData, quint32 destinationID)
{
    lock();

    State state;
    state.senderId = _id;
    state.destinationId = destinationID;
    state.bytesWrited = writeData.size();
    setState(state);

    char *to = static_cast<char*>(data());
    if (to) {
        memcpy(&to[DATA_POSITION], writeData.data(), std::min(_size, writeData.size()));
    }
    unlock();
}

bool SharedIO::open(const QString &key, qint32 size, QSharedMemory::AccessMode mode)
{
    bool result = false;
    _size = size;
    setKey(key);

    if (_id == SHARED_ID) {
        _id = QTime::currentTime().msecsSinceStartOfDay();
        qDebug() << "set id:" << _id;
    }

    if (attach(mode)) {
        qDebug() << "memory already attached";

        lock();
        Info info = getInfo();
        unlock();

        if (info.createSize != size)
            qDebug() << QString("target size(%1)) is not equal created size(%2). Target size set to %2")
                            .arg(size)
                            .arg(info.createSize);

        _size = info.createSize;

        result = true;
    } else if (create(size + sizeof(State), mode)) {
        qDebug() << "memory created size" <<size;
        State state;
        state.senderId = SHARED_ID;
        state.destinationId = SHARED_ID;
        Info info;
        info.creatorID = _id;
        info.createSize = size;
        lock();
        setState(state);
        setInfo(info);
        unlock();
        result = true;
    };

    if (!result) {
        qDebug() << "error connect to memory:" << errorString();
    } else {
        _timer->start(_checkInterval_msec);
    }

    return result;
}

quint32 SharedIO::id() const
{
    return _id;
}

void SharedIO::setId(const quint32 &id)
{
    _id = id;
}

void SharedIO::setCheckInterval(qint32 msec)
{
    _timer->setInterval(msec);
    if (!_paused)
        _timer->start();
}

void SharedIO::checkState()
{
    lock();
    State state = getState();
    unlock();

    if (state.senderId != _id &&
        (state.destinationId == SHARED_ID || state.destinationId == _id))
            emit readyRead();
}

SharedIO::State SharedIO::getState() const
{
    State state = {};
    const char *from = static_cast<const char*>(constData());
    if (from) {
        memcpy(&state, &from[STATE_POSITION], sizeof(State));
    }
    return state;
}

void SharedIO::setState(const SharedIO::State &state)
{
    char *to = static_cast<char*>(data());
    memcpy(&to[STATE_POSITION], &state, sizeof(State));
}

SharedIO::Info SharedIO::getInfo() const
{
    Info info = {};
    const char *from = static_cast<const char*>(constData());
    if (from) {
        memcpy(&info, &from[INFO_POSITION], sizeof(Info));
    }
    return info;
}

void SharedIO::setInfo(const SharedIO::Info &info)
{
    char *to = static_cast<char*>(data());
    memcpy(&to[INFO_POSITION], &info, sizeof(Info));
}

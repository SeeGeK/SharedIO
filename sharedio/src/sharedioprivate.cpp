#include "sharedioprivate.hpp"
#include "sharedio/sharedio.hpp"

#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>

class MemLocker
{
public:
    MemLocker(QSharedMemory *mem) : _mem(mem), _locked(true) {_mem->lock();};

    bool unlock()
    {
        _locked = false;
        return _mem->unlock();
    }

    bool lock()
    {
        if (!_locked)
            _locked = _mem->lock();
        return _locked;
    }

    ~MemLocker() {_mem->unlock();};

private:
    QSharedMemory * const _mem;
    bool _locked;
};

#define MEMLOCK MemLocker MEMORY_LOCKER(_memory.data())

SharedIOPrivate::SharedIOPrivate(SharedIO *shio, quint32 id)
    : q_ptr(shio)
    , _memory(new QSharedMemory)
    , _timer(new QTimer())
    , _syncTimer(new QTimer())
    , _id(id)
    , _creatorId(SharedIO::SHARED_ID)
    , _checkInterval_msec(10)
    , _maxWriteWait_msec(10000)
    , _internalSyncCounter(0)
    , _lastReadedPacketId(0)
{
    QObject::connect(_timer.data(), &QTimer::timeout, _memory.get(), [this](){check();});
    QObject::connect(_syncTimer.data(), &QTimer::timeout, _memory.get(), [this]()
                     {
                         sync();
                     });
}

SharedIOPrivate::~SharedIOPrivate()
{
    int atch = -1;
    if (_memory->isAttached()) {
        {
            MEMLOCK;
            auto info = get<Info>();
            auto sync = get<Sync>();
            info.attachedCount -= 1;
            atch = info.attachedCount;

            if (sync.sync && (sync.syncIniciatorId == _id))
            {
                sync.sync = false;
                sync.syncIniciatorId = 0;
                set(sync);
            }

            set(info);
        }
        _memory->detach();
    }
}

QPair<QByteArray, quint32> SharedIOPrivate::read() const
{
    QByteArray readData;

    MEMLOCK;

    State state{SharedIO::SHARED_ID, SharedIO::SHARED_ID, 0};
    Info info;

    const char *from = static_cast<const char*>(_memory->constData());
    if (from) {
        state = get<State>();
        info  = get<Info>();
        info.destinationReadedCount += 1;
        set(info);
        readData.resize(state.bytesWrited);
        memcpy(readData.data(), &from[dataOffset()], state.bytesWrited);
        _lastReadedPacketId = info.currentPacketId;
    }

    return {std::move(readData), state.senderId};
}

bool SharedIOPrivate::canWrite(MemLocker &locker)
{
    locker.lock();

    auto state = get<State>();
    auto info = get<Info>();
    auto sync = get<Sync>();

    qint32 attached = std::min(info.attachedCount, info.attachedCountOnWrite);

    if (!sync.sync)
    {
        if ( state.senderId == SharedIO::SHARED_ID ||
            (state.destinationId != SharedIO::SHARED_ID && info.destinationReadedCount > 0) ||
            (state.destinationId == SharedIO::SHARED_ID && info.destinationReadedCount >= attached-1))
        {
            return true;
        }
    }

    locker.unlock();
    return false;
}

quint64 SharedIOPrivate::write(const char *data, quint64 size, quint32 destinationId, bool force)
{
    if (_id == destinationId)
    {
        makeError("Destination ID can't be equal sender ID");
        return 0;
    }

    MEMLOCK;

    QElapsedTimer timer;
    timer.start();
    bool ok = false;
    while (timer.elapsed() < _maxWriteWait_msec)
    {
        if (!force && !canWrite(MEMORY_LOCKER))
        {
            check();
            QThread::msleep(_checkInterval_msec);
        }
        else
        {
            ok = true;
            break;
        }
    }

    if (!ok) {
        makeError("Previous messages in memory wasn't read by recipient");
        return 0;
    }

    auto state = get<State>();
    state.senderId = _id;
    state.destinationId = destinationId;
    state.bytesWrited = std::min(_allocatedSize, size);

    auto info = get<Info>();
    info.currentPacketId += 1;
    info.destinationReadedCount = 0;
    info.attachedCountOnWrite = info.attachedCount;
    set(info);
    set(state);

    char *to = static_cast<char*>(_memory->data());
    if (to) {
        memcpy(&to[dataOffset()], data, state.bytesWrited);
    }

    return state.bytesWrited;
}

bool SharedIOPrivate::open(const QString &key, quint64 size, QSharedMemory::AccessMode mode)
{
    bool result = false;
    _allocatedSize = size;
    _memory->setKey(key);

    if (_id == SharedIO::SHARED_ID) {
        _id = QTime::currentTime().msecsSinceStartOfDay();
        qInfo() << "Apply id:" << _id;
    }

    {
        //note: after crash shared memory may not be detached
        _memory->attach(mode);
        _memory->detach();
    }

    quint32 period = _checkInterval_msec;

    if (_memory->attach(mode)) {
        qInfo() << "memory already attached";

        Info info;
        {
            MEMLOCK;
            info = get<Info>();
            period = info.updatePeriod;
            info.attachedCount += 1;
            set(info);
        }

        if (info.allocSize != size) {
            makeError(QString("target size(%1)) is not equal created size(%2). Target size set to %2")
                                            .arg(size)
                                            .arg(info.allocSize));
        }

        _allocatedSize = info.allocSize;
        _creatorId = info.creatorID;

        result = true;
    } else if (_memory->create(size + sizeof(State) + sizeof(Info) + sizeof(Sync), mode)) {
        qInfo() << "Memory created size" <<size;
        State state;
        state.senderId = SharedIO::SHARED_ID;
        state.destinationId = SharedIO::SHARED_ID;
        Info info;
        info.creatorID = _id;
        info.allocSize = size;
        info.attachedCount = 1;
        info.currentPacketId = 0;
        info.updatePeriod = period;

        Sync sync = get<Sync>();
        sync.syncCounter = 0;
        sync.sync = 0;

        {
            MEMLOCK;
            set(state);
            set(info);
            set(sync);
        }
        result = true;
    };

    if (!result) {
        makeError("Error connect to memory:" + _memory->errorString());
    } else {
        updateTimers(period);
    }

    return result;
}

quint32 SharedIOPrivate::id() const
{
    return _id;
}

quint32 SharedIOPrivate::creatorId() const
{
    return _creatorId;
}

quint64 SharedIOPrivate::allocatedSize() const
{
    return _allocatedSize;
}

void SharedIOPrivate::setCheckInterval(qint32 msec)
{
    MEMLOCK;
    auto info = get<Info>();

    if (info.creatorID == _id)
    {
        info.updatePeriod = msec;
        set(info);
        updateTimers(msec);
    } else
        makeError("Only creator can change update period");
}

void SharedIOPrivate::setMaxWriteWait(qint32 msec)
{
    _maxWriteWait_msec = msec;
}

void SharedIOPrivate::check()
{
    Q_Q(SharedIO);

    State state;
    Info info;
    Sync sync;
    {
        MEMLOCK;

        state = get<State>();
        info  = get<Info>();
        sync  = get<Sync>();

        if (sync.sync && (_internalSyncCounter != sync.syncCounter) && (sync.syncIniciatorId != _id))
        {
            _internalSyncCounter = sync.syncCounter;
            info.attachedCount += 1;
            set(info);
        }

        if (_checkInterval_msec != info.updatePeriod)
        {
            updateTimers(info.updatePeriod);
        }
    }

    if (state.senderId != _id)
    {
        if (_lastReadedPacketId != info.currentPacketId) {
            if (state.destinationId == SharedIO::SHARED_ID || state.destinationId == _id)
                emit q->readyRead();
        }
    }
}

QSharedMemory *SharedIOPrivate::memory() const
{
    return _memory.data();
}

void SharedIOPrivate::sync()
{
    {
        MEMLOCK;

        auto info = get<Info>();
        auto sync = get<Sync>();

        if (sync.sync)
            return;

        sync.sync = true;
        sync.syncIniciatorId = _id;
        sync.syncCounter += 1;
        _internalSyncCounter = sync.syncCounter;
        info.attachedCount = 1;

        set(sync);
        set(info);
    }

    QTimer::singleShot(_checkInterval_msec * 3, _memory.data(), [this]()
                       {
                           {
                               MEMLOCK;
                               auto sync = get<Sync>();
                               sync.sync = false;
                               sync.syncIniciatorId = 0;
                               set(sync);
                           }
                       });
}

void SharedIOPrivate::updateTimers(quint32 msec)
{
    _checkInterval_msec = msec;
    _timer->start(_checkInterval_msec);
    _syncTimer->start(_checkInterval_msec * 100);
}

void SharedIOPrivate::makeError(const QString &text)
{
    Q_Q(SharedIO);
    emit q->errorOccurred(text);
}

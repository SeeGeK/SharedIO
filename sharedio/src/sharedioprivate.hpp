#ifndef SHAREDIOPRIVATE_HPP
#define SHAREDIOPRIVATE_HPP

#include <QSharedMemory>

class SharedIO;
class QTimer;

class MemLocker;

class SharedIOPrivate
{
    Q_DECLARE_PUBLIC(SharedIO);
public:
    SharedIOPrivate(SharedIO *shio, quint32 id);
    ~SharedIOPrivate();

    QPair<QByteArray, quint32> read() const;
    quint64 write(const char *data, quint64 size, quint32 destinationId, bool force);

    bool open(const QString &key, quint64 size, QSharedMemory::AccessMode mode);

    quint32 id() const;
    quint32 creatorId() const;
    quint64 allocatedSize() const;

    void setCheckInterval(qint32 msec);
    void setMaxWriteWait(qint32 msec);

    quint32 getSenderId() const;
    quint32 getDestinationId() const;

    SharedIO *q_ptr;

    QScopedPointer<QSharedMemory> _memory;
    QScopedPointer<QTimer> _timer;
    QScopedPointer<QTimer> _syncTimer;

    struct State
    {
        quint32 senderId;
        quint32 destinationId;
        quint64 bytesWrited;
    };

    struct Info
    {
        quint32 creatorID;
        quint64 allocSize;
        qint32  attachedCount;
        quint64 currentPacketId;
        //
        qint32 attachedCountOnWrite;
        qint32 destinationReadedCount;
        //
        quint32 updatePeriod;
    };

    struct Sync
    {
        bool sync;
        quint32 syncIniciatorId;
        quint32 syncCounter;
    };

    quint32 _id;
    quint32 _creatorId;
    quint64 _allocatedSize;
    quint32 _checkInterval_msec;
    quint32 _maxWriteWait_msec;
    quint64 _internalSyncCounter;
    mutable quint64 _lastReadedPacketId;

    void check();

    QSharedMemory *memory() const;

    bool canWrite(MemLocker &locker);

    void sync();

    void updateTimers(quint32 msec);

    void makeError(const QString &text);

    template<typename T> constexpr size_t offset() const;
    template<typename T> T get() const;
    template<typename T> void set(const T &d) const;

    size_t dataOffset() const;
};

template<> constexpr size_t SharedIOPrivate::offset<SharedIOPrivate::State>() const {return 0; };
template<> constexpr size_t SharedIOPrivate::offset<SharedIOPrivate::Info>()  const {return offset<State>() + sizeof(State); };
template<> constexpr size_t SharedIOPrivate::offset<SharedIOPrivate::Sync>()  const {return offset<Info>()  + sizeof(Info); };

template<typename T>
inline T SharedIOPrivate::get() const
{
    T sync = {};
    const char *from = static_cast<const char*>(_memory->constData());
    if (from) {
        memcpy(&sync, &from[offset<T>()], sizeof(T));
    }
    return sync;
}

template<typename T>
inline void SharedIOPrivate::set(const T &d) const
{
    char *to = static_cast<char*>(_memory->data());
    memcpy(&to[offset<T>()], &d, sizeof(T));
}

inline size_t SharedIOPrivate::dataOffset() const  {return offset<Sync>() + sizeof(Sync);}

#endif

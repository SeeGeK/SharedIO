#include "sharedio/sharedio.hpp"
#include "sharedioprivate.hpp"

SharedIO::SharedIO(quint32 id, QObject *parent)
    : QObject(parent)
    , d_ptr(new SharedIOPrivate(this, id))
{
}

SharedIO::~SharedIO()
{
}

SharedIO::ReceivePacket SharedIO::read() const
{
    Q_D(const SharedIO);
    return d->read();
}

quint64 SharedIO::write(const QByteArray &buffer, quint32 destinationId)
{
    return write(buffer.data(), buffer.size(), destinationId);
}

quint64 SharedIO::write(const char *data, quint64 size, quint32 destinationId)
{
    Q_D(SharedIO);
    return d->write(data, size, destinationId, false);
}


quint64 SharedIO::forceWrite(const QByteArray &buffer, quint32 destinationId)
{
    return forceWrite(buffer.data(), buffer.size(), destinationId);
}

quint64 SharedIO::forceWrite(const char *data, quint64 size, quint32 destinationId)
{
    Q_D(SharedIO);
    return d->write(data, size, destinationId, true);
}

bool SharedIO::open(const QString &key, quint64 size, QSharedMemory::AccessMode mode)
{
    Q_D(SharedIO);
    return d->open(key, size, mode);
}

quint32 SharedIO::id() const
{
    Q_D(const SharedIO);
    return d->id();
}

quint32 SharedIO::creatorId() const
{
    Q_D(const SharedIO);
    return d->creatorId();
}

quint64 SharedIO::allocatedSize() const
{
    Q_D(const SharedIO);
    return d->allocatedSize();
}

void SharedIO::setCheckInterval(qint32 msec)
{
    Q_D(SharedIO);
    d->setCheckInterval(msec);
}

void SharedIO::setMaxWriteWait(qint32 msec)
{
    Q_D(SharedIO);
    d->setMaxWriteWait(msec);
}

QSharedMemory *SharedIO::memory() const
{
    Q_D(const SharedIO);
    return d->memory();
}

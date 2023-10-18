#ifndef SHAREDIO_HPP
#define SHAREDIO_HPP

#include <QObject>
#include <QSharedMemory>

class SharedIOPrivate;

class SharedIO : public QObject
{
public:
    static constexpr quint32 SHARED_ID = quint32(-1);

    Q_OBJECT

public:
    SharedIO(quint32 id = SHARED_ID, QObject *parent = nullptr);
    ~SharedIO();

    bool open(const QString &key, quint64 size = 4096,
              QSharedMemory::AccessMode mode = QSharedMemory::AccessMode::ReadWrite);

    using ReceivePacket = QPair<QByteArray, quint32>; //<data, senderId>

    ReceivePacket read() const;

    quint64 write(const QByteArray &buffer, quint32 destinationId);
    quint64 write(const char *data, quint64 size, quint32 destinationId);

    quint64 forceWrite(const QByteArray &buffer, quint32 destinationId);
    quint64 forceWrite(const char *data, quint64 size, quint32 destinationId);

    quint32 id() const;
    quint32 creatorId() const;
    quint64 allocatedSize() const;
    void setCheckInterval(qint32 msec);
    void setMaxWriteWait(qint32 msec);

    QSharedMemory *memory() const;

signals:
    void readyRead();
    void errorOccurred(QString);

private:
    Q_DECLARE_PRIVATE(SharedIO);
    QScopedPointer<SharedIOPrivate> d_ptr;
};

#endif // SHAREDIO_HPP

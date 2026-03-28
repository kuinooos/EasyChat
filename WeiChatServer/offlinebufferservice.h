#pragma once

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QFutureWatcher>
#include <QString>
#include <QList>

class OfflineBufferService : public QObject
{
    Q_OBJECT
public:
    explicit OfflineBufferService(QObject *parent = nullptr);
    ~OfflineBufferService() override;

    void start();
    void stop();

    void enqueueOfflineMessage(const QString &sender, const QString &receiver, const QString &message);
    void enqueueOfflineFile(const QString &sender, const QString &receiver, const QString &filePath);

private slots:
    void onFlushTimer();

private:
    struct Envelope {
        QString sender;
        QString receiver;
        QString content;
        QString kind;
        QString idempotencyKey;
        QString sendTimeText;
    };

    static QString currentTimeText();
    static QString makeIdempotencyKey(const Envelope &env);

    QByteArray serializeEnvelope(const Envelope &env) const;
    bool parseEnvelope(const QByteArray &payload, Envelope &env) const;

    void appendFailsafeLine(const QByteArray &payload);
    QList<QByteArray> takeFailsafeLines(int maxCount);

    QList<Envelope> buildFlushBatch(int maxCount);
    bool insertBatchToSqlServer(const QList<Envelope> &batch) const;
    void flushOnce();

private:
    QTimer *m_flushTimer = nullptr;
    QFutureWatcher<void> *m_flushWatcher = nullptr;
    QString m_failsafePath;

    int m_batchSize = 500;
    int m_flushIntervalMs = 5000;

    mutable QMutex m_failsafeMutex;
};

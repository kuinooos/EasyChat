#include "offlinebufferservice.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent/QtConcurrent>
#include "dbconnectionmanager.h"

namespace {
}

OfflineBufferService::OfflineBufferService(QObject *parent)
    : QObject(parent)
{
    const int batchSize = qEnvironmentVariableIntValue("EASYCHAT_OFFLINE_BATCH_SIZE");
    if (batchSize > 0) {
        m_batchSize = batchSize;
    }
    const int flushMs = qEnvironmentVariableIntValue("EASYCHAT_OFFLINE_FLUSH_MS");
    if (flushMs > 0) {
        m_flushIntervalMs = flushMs;
    }

    const QString baseDir = QCoreApplication::applicationDirPath();
    m_failsafePath = QDir(baseDir).filePath(QStringLiteral("offline_failsafe.jsonl"));

    m_flushTimer = new QTimer(this);
    connect(m_flushTimer, &QTimer::timeout, this, &OfflineBufferService::onFlushTimer);

    m_flushWatcher = new QFutureWatcher<void>(this);
}

OfflineBufferService::~OfflineBufferService()
{
    stop();
}

void OfflineBufferService::start()
{
    if (!m_flushTimer->isActive()) {
        m_flushTimer->start(m_flushIntervalMs);
    }
}

void OfflineBufferService::stop()
{
    if (m_flushTimer->isActive()) {
        m_flushTimer->stop();
    }

    if (m_flushWatcher->isRunning()) {
        m_flushWatcher->future().waitForFinished();
    }

    flushOnce();
}

QString OfflineBufferService::currentTimeText()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
}

QString OfflineBufferService::makeIdempotencyKey(const Envelope &env)
{
    const QByteArray src = (env.sender + QStringLiteral("|") + env.receiver + QStringLiteral("|")
                            + env.kind + QStringLiteral("|") + env.sendTimeText + QStringLiteral("|") + env.content).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(src, QCryptographicHash::Md5).toHex());
}

QByteArray OfflineBufferService::serializeEnvelope(const Envelope &env) const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("sender"), env.sender);
    obj.insert(QStringLiteral("receiver"), env.receiver);
    obj.insert(QStringLiteral("content"), env.content);
    obj.insert(QStringLiteral("kind"), env.kind);
    obj.insert(QStringLiteral("idempotency_key"), env.idempotencyKey);
    obj.insert(QStringLiteral("send_time"), env.sendTimeText);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

bool OfflineBufferService::parseEnvelope(const QByteArray &payload, Envelope &env) const
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    const QJsonObject obj = doc.object();
    env.sender = obj.value(QStringLiteral("sender")).toString();
    env.receiver = obj.value(QStringLiteral("receiver")).toString();
    env.content = obj.value(QStringLiteral("content")).toString();
    env.kind = obj.value(QStringLiteral("kind")).toString();
    env.idempotencyKey = obj.value(QStringLiteral("idempotency_key")).toString();
    env.sendTimeText = obj.value(QStringLiteral("send_time")).toString();
    return !env.sender.isEmpty() && !env.receiver.isEmpty() && !env.sendTimeText.isEmpty();
}

void OfflineBufferService::enqueueOfflineMessage(const QString &sender, const QString &receiver, const QString &message)
{
    Envelope env;
    env.sender = sender;
    env.receiver = receiver;
    env.content = message;
    env.kind = QStringLiteral("text");
    env.sendTimeText = currentTimeText();
    env.idempotencyKey = makeIdempotencyKey(env);

    const QByteArray payload = serializeEnvelope(env);
    appendFailsafeLine(payload);
}

void OfflineBufferService::enqueueOfflineFile(const QString &sender, const QString &receiver, const QString &filePath)
{
    QtConcurrent::run([this, sender, receiver, filePath]() {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QFile::remove(filePath);
            return;
        }

        const QByteArray fileData = file.readAll();
        file.close();
        QFile::remove(filePath);

        Envelope env;
        env.sender = sender;
        env.receiver = receiver;
        env.content = QString::fromLatin1(fileData.toBase64());
        env.kind = QStringLiteral("file_base64");
        env.sendTimeText = currentTimeText();
        env.idempotencyKey = makeIdempotencyKey(env);

        const QByteArray payload = serializeEnvelope(env);
        appendFailsafeLine(payload);
    });
}

void OfflineBufferService::onFlushTimer()
{
    if (m_flushWatcher->isRunning()) {
        return;
    }

    m_flushWatcher->setFuture(QtConcurrent::run([this]() {
        flushOnce();
    }));
}

void OfflineBufferService::appendFailsafeLine(const QByteArray &payload)
{
    QMutexLocker locker(&m_failsafeMutex);
    QFile file(m_failsafePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "写入failsafe失败" << m_failsafePath;
        return;
    }
    file.write(payload);
    file.write("\n");
}

QList<QByteArray> OfflineBufferService::takeFailsafeLines(int maxCount)
{
    QList<QByteArray> out;
    QMutexLocker locker(&m_failsafeMutex);

    QFile file(m_failsafePath);
    if (!file.exists()) {
        return out;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return out;
    }

    QList<QByteArray> allLines;
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line.endsWith("\r\n")) {
            line.chop(2);
        } else if (line.endsWith('\n')) {
            line.chop(1);
        }
        if (!line.isEmpty()) {
            allLines.push_back(line);
        }
    }
    file.close();

    const int takeCount = qMin(maxCount, allLines.size());
    for (int i = 0; i < takeCount; ++i) {
        out.push_back(allLines[i]);
    }

    QFile rewrite(m_failsafePath);
    if (!rewrite.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return out;
    }
    for (int i = takeCount; i < allLines.size(); ++i) {
        rewrite.write(allLines[i]);
        rewrite.write("\n");
    }
    rewrite.close();

    return out;
}

QList<OfflineBufferService::Envelope> OfflineBufferService::buildFlushBatch(int maxCount)
{
    QList<Envelope> batch;
    batch.reserve(maxCount);

    const QList<QByteArray> failsafeLines = takeFailsafeLines(maxCount);
    for (const QByteArray &line : failsafeLines) {
        Envelope env;
        if (parseEnvelope(line, env)) {
            batch.push_back(env);
        }
    }

    return batch;
}

bool OfflineBufferService::insertBatchToSqlServer(const QList<Envelope> &batch) const
{
    if (batch.isEmpty()) {
        return true;
    }

    QSqlDatabase db = DbConnectionManager::instance().acquire();
    if (!db.open()) {
        qWarning() << "离线批量落库连接失败" << db.lastError().text();
        return false;
    }

    if (!db.transaction()) {
        qWarning() << "离线批量事务开启失败" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO messages(sender_id,receiver_id,message_text,send_time,is_read) "
                                 "VALUES(:username,:friendname,:content,:send_time,0)"));

    for (const Envelope &env : batch) {
        query.bindValue(QStringLiteral(":username"), env.sender);
        query.bindValue(QStringLiteral(":friendname"), env.receiver);
        query.bindValue(QStringLiteral(":content"), env.content);
        query.bindValue(QStringLiteral(":send_time"), QDateTime::fromString(env.sendTimeText, QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
        if (!query.exec()) {
            qWarning() << "离线批量落库失败" << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    const bool commitOk = db.commit();
    if (!commitOk) {
        qWarning() << "离线批量事务提交失败" << db.lastError().text();
    }

    return commitOk;
}

void OfflineBufferService::flushOnce()
{
    const QList<Envelope> batch = buildFlushBatch(m_batchSize);
    if (batch.isEmpty()) {
        return;
    }

    if (insertBatchToSqlServer(batch)) {
        return;
    }

    for (const Envelope &env : batch) {
        const QByteArray payload = serializeEnvelope(env);
        appendFailsafeLine(payload);
    }
}

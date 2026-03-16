#ifndef BENCHMARKCLIENT_H
#define BENCHMARKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QVector>
#include <chrono>

// ── 模拟单个客户端连接 ──────────────────────────────────────────────
class BenchmarkClient : public QObject {
    Q_OBJECT
public:
    explicit BenchmarkClient(QObject *parent = nullptr)
        : QObject(parent), m_socket(new QTcpSocket(this))
    {
        connect(m_socket, &QTcpSocket::connected, this, [this]() { emit connected(); });
        connect(m_socket, &QTcpSocket::readyRead, this, &BenchmarkClient::onReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(m_socket, &QAbstractSocket::errorOccurred,
                this, [this](QAbstractSocket::SocketError) { emit connectFailed(); });
#else
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                this, [this](QAbstractSocket::SocketError) { emit connectFailed(); });
#endif
    }

    // ── 协议操作 ────────────────────────────────────────────────────
    void connectToServer(const QString &host, quint16 port) {
        m_socket->connectToHost(host, port);
    }

    void sendOnline(const QString &username) {
        m_username = username;
        m_socket->write(("ONLINE::" + username + "\n").toUtf8());
    }

    // 发消息并嵌入微秒时间戳用于延迟测量
    void sendTimestampMessage(const QString &receiver) {
        qint64 us = microsNow();
        m_socket->write((m_username + "::" + receiver + "::BENCH::" +
                         QString::number(us) + "\n").toUtf8());
    }

    // 发送文件（header + 随机二进制体）
    void sendFileData(const QString &receiver, const QString &fileName, qint64 fileSize) {
        QString hdr = "FILE::" + fileName + "::" + QString::number(fileSize)
                    + "::" + m_username + "::" + receiver + "\n";
        m_socket->write(hdr.toUtf8());
        qint64 sent = 0;
        while (sent < fileSize) {
            int chunk = static_cast<int>(qMin<qint64>(65536, fileSize - sent));
            m_socket->write(QByteArray(chunk, 'X'));
            sent += chunk;
        }
    }

    // ── 查询 ────────────────────────────────────────────────────────
    bool isConnected() const {
        return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
    }
    const QString &username() const { return m_username; }
    int messagesReceived() const    { return m_msgCount; }
    const QVector<double> &latencySamples() const { return m_latencies; }

    // 微秒级单调时钟（进程内一致）
    static qint64 microsNow() {
        using namespace std::chrono;
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    }

signals:
    void connected();
    void connectFailed();
    void messageReceived();
    void fileFullyReceived(qint64 bytes);

private slots:
    void onReadyRead() {
        m_buf.append(m_socket->readAll());
        while (true) {
            // 如果正在接收文件二进制体
            if (m_recvFile) {
                qint64 need = m_fileExp - m_fileGot;
                qint64 have = m_buf.size();
                qint64 take = qMin(need, have);
                m_buf.remove(0, static_cast<int>(take));
                m_fileGot += take;
                if (m_fileGot >= m_fileExp) {
                    m_recvFile = false;
                    emit fileFullyReceived(m_fileExp);
                    continue;
                }
                break; // 等更多数据
            }
            int nl = m_buf.indexOf('\n');
            if (nl < 0) break;
            QString line = QString::fromUtf8(m_buf.left(nl)).trimmed();
            m_buf.remove(0, nl + 1);
            if (line.isEmpty()) continue;

            QStringList p = line.split("::");
            // MESSAGE::sender::content...
            if (p.size() >= 3 && p[0] == "MESSAGE") {
                m_msgCount++;
                QString content = p.mid(2).join("::");
                if (content.startsWith("BENCH::")) {
                    bool ok;
                    qint64 ts = content.mid(7).toLongLong(&ok);
                    if (ok) m_latencies.append((microsNow() - ts) / 1000.0); // ms
                }
                emit messageReceived();
            }
            // FILE::name::size::sender::receiver
            else if (p.size() >= 5 && p[0] == "FILE") {
                m_fileExp = p[2].toLongLong();
                m_fileGot = 0;
                m_recvFile = true;
            }
        }
    }

private:
    QTcpSocket *m_socket;
    QByteArray  m_buf;
    QString     m_username;
    int         m_msgCount = 0;
    QVector<double> m_latencies;
    bool   m_recvFile = false;
    qint64 m_fileExp  = 0;
    qint64 m_fileGot  = 0;
};

#endif // BENCHMARKCLIENT_H

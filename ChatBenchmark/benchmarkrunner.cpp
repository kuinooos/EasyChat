#include "benchmarkrunner.h"
#include <QCoreApplication>
#include <QTimer>
#include <QtMath>
#include <algorithm>

BenchmarkRunner::BenchmarkRunner(const Config &cfg, QObject *parent)
    : QObject(parent), m_cfg(cfg) {}

// ═══════════════════════════════════════════════════════════════════
void BenchmarkRunner::start() {
    qDebug().noquote() << QString::fromUtf8(
        "\n"
        "═══════════════════════════════════════════════\n"
        "       EasyChat 性能基准测试\n"
        "═══════════════════════════════════════════════\n"
        " 目标:  %1:%2\n"
        " 连接数: %3  |  消息/对: %4  |  文件: %5 MB\n"
        "═══════════════════════════════════════════════")
        .arg(m_cfg.host).arg(m_cfg.port)
        .arg(m_cfg.connections).arg(m_cfg.messagesPerPair).arg(m_cfg.fileSizeMB);
    phaseConnect();
}

// ── 阶段 1：并发连接 ───────────────────────────────────────────────
void BenchmarkRunner::phaseConnect() {
    qDebug().noquote() << QString::fromUtf8("\n[1/4] 并发连接测试: 建立 %1 个连接...")
                          .arg(m_cfg.connections);

    for (int i = 0; i < m_cfg.connections; ++i) {
        auto *c = new BenchmarkClient(this);
        m_clients.append(c);

        connect(c, &BenchmarkClient::connected, this, [this, c]() {
            if (m_connectDone) return;
            m_connectSuccess++;
            m_connectResponded++;
            m_connected.append(c);
            if (m_connectResponded >= m_cfg.connections) {
                m_connectDone = true;
                phaseRegister();
            }
        });
        connect(c, &BenchmarkClient::connectFailed, this, [this]() {
            if (m_connectDone) return;
            m_connectResponded++;
            if (m_connectResponded >= m_cfg.connections) {
                m_connectDone = true;
                phaseRegister();
            }
        });
        c->connectToServer(m_cfg.host, m_cfg.port);
    }

    // 超时保底
    QTimer::singleShot(15000, this, [this]() {
        if (!m_connectDone) {
            m_connectDone = true;
            qDebug().noquote() << QString::fromUtf8("  连接超时，继续...");
            phaseRegister();
        }
    });
}

// ── 阶段 2：注册用户 ───────────────────────────────────────────────
void BenchmarkRunner::phaseRegister() {
    qDebug().noquote() << QString::fromUtf8("  结果: %1 / %2 连接成功")
                          .arg(m_connectSuccess).arg(m_cfg.connections);
    qDebug().noquote() << QString::fromUtf8("\n[2/4] 注册用户...");

    for (int i = 0; i < m_connected.size(); ++i)
        m_connected[i]->sendOnline("bench_user_" + QString::number(i));

    // 等服务端处理完注册（稍微等久一点，防止大批量并发没处理完就发消息导致被当成离线用户）
    QTimer::singleShot(5000, this, &BenchmarkRunner::phaseMessageTest);
}

// ── 阶段 3：消息吞吐 & 延迟 ────────────────────────────────────────
void BenchmarkRunner::phaseMessageTest() {
    int numPairs = m_connected.size() / 2;
    if (numPairs == 0) {
        qDebug().noquote() << QString::fromUtf8("  连接不足 2 个，跳过消息测试");
        phaseFileTest();
        return;
    }

    m_totalMsgExpected = numPairs * m_cfg.messagesPerPair;
    qDebug().noquote() << QString::fromUtf8("\n[3/4] 消息吞吐测试: %1 对用户, 共 %2 条消息...")
                          .arg(numPairs).arg(m_totalMsgExpected);

    // 订阅接收方的 messageReceived 信号
    for (int i = 0; i < numPairs; ++i) {
        BenchmarkClient *receiver = m_connected[2 * i + 1];
        connect(receiver, &BenchmarkClient::messageReceived, this, [this]() {
            m_totalMsgReceived++;
            if (!m_msgDone && m_totalMsgReceived >= m_totalMsgExpected)
                onMessageTestDone();
        });
    }

    m_msgTimer.start();

    // 所有发送方同时开火
    for (int i = 0; i < numPairs; ++i) {
        BenchmarkClient *sender   = m_connected[2 * i];
        BenchmarkClient *receiver = m_connected[2 * i + 1];
        for (int j = 0; j < m_cfg.messagesPerPair; ++j)
            sender->sendTimestampMessage(receiver->username());
    }

    // 超时保底
    QTimer::singleShot(60000, this, [this]() {
        if (!m_msgDone) {
            qDebug().noquote() << QString::fromUtf8("  消息测试超时, 已收到 %1 / %2")
                                  .arg(m_totalMsgReceived).arg(m_totalMsgExpected);
            onMessageTestDone();
        }
    });
}

void BenchmarkRunner::onMessageTestDone() {
    if (m_msgDone) return;
    m_msgDone = true;

    double elapsedSec = m_msgTimer.elapsed() / 1000.0;
    m_msgThroughput = (elapsedSec > 0) ? m_totalMsgReceived / elapsedSec : 0;

    // 收集所有接收方的延迟样本
    QVector<double> all;
    int numPairs = m_connected.size() / 2;
    for (int i = 0; i < numPairs; ++i) {
        const auto &s = m_connected[2 * i + 1]->latencySamples();
        all.append(s);
    }

    if (!all.isEmpty()) {
        std::sort(all.begin(), all.end());
        m_latP50 = all[all.size() * 50 / 100];
        m_latP99 = all[all.size() * 99 / 100];
        double sum = 0;
        for (double v : all) sum += v;
        m_latAvg = sum / all.size();
    }

    qDebug().noquote() << QString::fromUtf8("  完成: %1 条消息, %.1f 秒")
                          .arg(m_totalMsgReceived).arg(elapsedSec);
    phaseFileTest();
}

// ── 阶段 4：文件传输 ───────────────────────────────────────────────
void BenchmarkRunner::phaseFileTest() {
    if (m_connected.size() < 2) {
        qDebug().noquote() << QString::fromUtf8("  连接不足, 跳过文件测试");
        printReport();
        return;
    }

    qint64 fileBytes = static_cast<qint64>(m_cfg.fileSizeMB) * 1024 * 1024;
    qDebug().noquote() << QString::fromUtf8("\n[4/4] 文件传输测试: %1 MB...").arg(m_cfg.fileSizeMB);

    BenchmarkClient *sender   = m_connected[0];
    BenchmarkClient *receiver = m_connected[1];

    connect(receiver, &BenchmarkClient::fileFullyReceived, this, [this](qint64 bytes) {
        Q_UNUSED(bytes);
        if (!m_fileDone) onFileTestDone();
    });

    m_fileTimer.start();
    sender->sendFileData(receiver->username(), "benchmark_test.bin", fileBytes);

    // 超时：按 1MB/s 最低速度计算
    int timeout = qMax(30000, m_cfg.fileSizeMB * 2000);
    QTimer::singleShot(timeout, this, [this]() {
        if (!m_fileDone) {
            qDebug().noquote() << QString::fromUtf8("  文件测试超时");
            onFileTestDone();
        }
    });
}

void BenchmarkRunner::onFileTestDone() {
    if (m_fileDone) return;
    m_fileDone = true;

    double elapsedSec = m_fileTimer.elapsed() / 1000.0;
    m_fileMBps = (elapsedSec > 0) ? m_cfg.fileSizeMB / elapsedSec : 0;

    qDebug().noquote() << QString::fromUtf8("  完成: %.2f 秒").arg(elapsedSec);
    printReport();
}

// ── 最终报告 ────────────────────────────────────────────────────────
void BenchmarkRunner::printReport() {
    QString report = QString::fromUtf8(
        "\n"
        "═══════════════════════════════════════════════\n"
        "       EasyChat 性能基准测试报告\n"
        "═══════════════════════════════════════════════\n"
        "\n"
        " [并发连接]  %1 / %2 成功  (%3%)\n"
        "\n"
        " [消息吞吐]  %4 msg/s  (共 %5 条)\n"
        " [消息延迟]  P50 = %6 ms | P99 = %7 ms | Avg = %8 ms\n"
        "\n"
        " [文件传输]  %9 MB/s  (%10 MB 文件)\n"
        "\n"
        "═══════════════════════════════════════════════\n")
        .arg(m_connectSuccess).arg(m_cfg.connections)
        .arg(m_connectSuccess * 100.0 / qMax(1, m_cfg.connections), 0, 'f', 1)
        .arg(m_msgThroughput, 0, 'f', 0)
        .arg(m_totalMsgReceived)
        .arg(m_latP50, 0, 'f', 2)
        .arg(m_latP99, 0, 'f', 2)
        .arg(m_latAvg, 0, 'f', 2)
        .arg(m_fileMBps, 0, 'f', 1)
        .arg(m_cfg.fileSizeMB);

    qDebug().noquote() << report;
    QCoreApplication::quit();
}

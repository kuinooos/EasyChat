#ifndef BENCHMARKRUNNER_H
#define BENCHMARKRUNNER_H

#include <QObject>
#include <QVector>
#include <QElapsedTimer>
#include "benchmarkclient.h"

class BenchmarkRunner : public QObject {
    Q_OBJECT
public:
    struct Config {
        QString host       = "::1";
        quint16 port       = 7777;
        int     connections      = 200;
        int     messagesPerPair  = 500;
        int     fileSizeMB       = 10;
    };

    explicit BenchmarkRunner(const Config &cfg, QObject *parent = nullptr);
    void start();

private:
    void phaseConnect();
    void phaseRegister();
    void phaseMessageTest();
    void onMessageTestDone();
    void phaseFileTest();
    void onFileTestDone();
    void printReport();

    Config m_cfg;
    QVector<BenchmarkClient*> m_clients;
    QVector<BenchmarkClient*> m_connected;

    // 连接阶段
    int  m_connectResponded = 0;
    int  m_connectSuccess   = 0;
    bool m_connectDone      = false;

    // 消息阶段
    int  m_totalMsgExpected = 0;
    int  m_totalMsgReceived = 0;
    bool m_msgDone          = false;
    QElapsedTimer m_msgTimer;

    // 文件阶段
    bool m_fileDone = false;
    QElapsedTimer m_fileTimer;

    // 结果
    double m_msgThroughput = 0;
    double m_latP50 = 0, m_latP99 = 0, m_latAvg = 0;
    double m_fileMBps = 0;
};

#endif // BENCHMARKRUNNER_H

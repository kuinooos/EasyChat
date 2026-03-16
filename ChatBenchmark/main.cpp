#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include "benchmarkrunner.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ChatBenchmark");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(QString::fromUtf8("EasyChat 服务端性能基准测试工具"));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption({{"H", "host"},  "Server host (default: ::1)",        "host", "::1"});
    parser.addOption({{"p", "port"},  "Server port (default: 7777)",       "port", "7777"});
    parser.addOption({{"c", "connections"}, "Concurrent connections (default: 200)", "N", "200"});
    parser.addOption({{"m", "messages"},    "Messages per pair (default: 500)",      "N", "500"});
    parser.addOption({{"f", "filesize"},    "File size in MB (default: 10)",         "MB", "10"});

    parser.process(app);

    BenchmarkRunner::Config cfg;
    cfg.host            = parser.value("host");
    cfg.port            = parser.value("port").toUShort();
    cfg.connections     = parser.value("connections").toInt();
    cfg.messagesPerPair = parser.value("messages").toInt();
    cfg.fileSizeMB      = parser.value("filesize").toInt();

    BenchmarkRunner runner(cfg);
    QTimer::singleShot(0, &runner, &BenchmarkRunner::start);

    return app.exec();
}

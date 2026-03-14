#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <atomic>

class WorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit WorkerThread(QObject *parent = nullptr) : QThread(parent) {}

    // 线程入口：只跑事件循环，等待事件投递
    void run() override { exec(); }

    // 当前管理的连接数
    int connectionCount() const { return m_count.load(); }

    void incrementCount() { ++m_count; }
    void decrementCount() { --m_count; }

private:
    std::atomic<int> m_count{0};
};

#endif // WORKERTHREAD_H

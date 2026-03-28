#pragma once

#include <QSqlDatabase>
#include <QThreadStorage>
#include <QMutex>
#include <atomic>

class DbConnectionManager
{
public:
    static DbConnectionManager &instance();

    // Returns a thread-local SQL Server connection. The first call per thread creates and opens it.
    QSqlDatabase acquire();

private:
    DbConnectionManager() = default;
    DbConnectionManager(const DbConnectionManager &) = delete;
    DbConnectionManager &operator=(const DbConnectionManager &) = delete;

private:
    QThreadStorage<QString> m_connectionName;
    QMutex m_mutex;
    std::atomic_uint m_counter{0};
};

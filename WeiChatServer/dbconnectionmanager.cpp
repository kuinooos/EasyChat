#include "dbconnectionmanager.h"

#include <QSqlError>
#include <QDebug>
#include <QThread>

namespace {
QString sqlServerConnectionString()
{
    return QStringLiteral("Driver={SQL Server};Server=(local);Database=ChatApp;Trusted_Connection=yes;");
}
}

DbConnectionManager &DbConnectionManager::instance()
{
    static DbConnectionManager mgr;
    return mgr;
}

QSqlDatabase DbConnectionManager::acquire()
{
    if (!m_connectionName.hasLocalData()) {
        const quintptr tid = reinterpret_cast<quintptr>(QThread::currentThreadId());
        const uint seq = m_counter.fetch_add(1, std::memory_order_relaxed);
        m_connectionName.setLocalData(QStringLiteral("easychat_sql_%1_%2").arg(tid).arg(seq));
    }

    const QString name = m_connectionName.localData();

    {
        QMutexLocker locker(&m_mutex);
        if (!QSqlDatabase::contains(name)) {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QODBC"), name);
            db.setDatabaseName(sqlServerConnectionString());
        }
    }

    QSqlDatabase db = QSqlDatabase::database(name);
    if (!db.isOpen() && !db.open()) {
        qWarning() << "Database connection failed:" << db.lastError().text();
    }
    return db;
}

#include<QTcpServer>
#include<QTcpSocket>
#include<QHostAddress>
#include<QSqlDatabase>
#include<QSqlError>
#include<QSqlQuery>
#include<QStringList>
#include "dbconnectionmanager.h"

class FriendServer : public QTcpServer{
    Q_OBJECT
public:
    explicit FriendServer(QObject* parent = nullptr) : QTcpServer(parent){
        if (this->listen(QHostAddress::AnyIPv6, 7000)) {
            qDebug() << "FriendServer started on" << serverAddress().toString() << "port 7000 (IPv6)";
        } else {
            qWarning() << "FriendServer IPv6 listen failed:" << errorString() << "falling back to IPv4";
            if (this->listen(QHostAddress::Any, 7000)) {
                qDebug() << "FriendServer started on" << serverAddress().toString() << "port 7000 (IPv4 fallback)";
            } else {
                qWarning() << "FriendServer failed to start on both IPv6 and IPv4:" << errorString();
            }
        }
    };
    ~FriendServer() = default;

protected:
    void incomingConnection(qintptr socketDescriptor) override {
        auto *socket = new QTcpSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            qWarning() << "Failed to set socket descriptor for friend request" << socketDescriptor;
            delete socket;
            return;
        }

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            const QString response = processRequest(QString::fromUtf8(socket->readAll()).trimmed());
            if (!response.isNull()) {
                socket->write(response.toUtf8());
            }
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
private:
    static QString processRequest(const QString &message) {
        QSqlDatabase db = DbConnectionManager::instance().acquire();
        if (!db.isOpen()) {
            return QString();
        }

        QString response;
        const QStringList parts = message.split("::");
        if (!parts.isEmpty()) {
            const QString messageType = parts[0];
            const QString actor = parts.size() > 1 ? parts[1] : QString();
            const QString target = parts.size() > 2 ? parts[2] : actor;

            if (messageType == "ADD") {
                response = handleFriendADD(db, actor, target);
            } else if (messageType == "FRIEND") {
                response = handleusersDisplayFRIEND(db, actor).join(",");
            } else if (messageType == "ARGEE") {
                response = handleFriendARGEE(db, actor, target);
            } else if (messageType == "REFUSE") {
                response = handleFriendREFUSE(db, actor, target);
            } else if (messageType == "NOWMYFRIEND") {
                response = handleusersFRIENDNOW(db, actor).join(",");
            } else if (messageType == "FRIENDINFO") {
                response = handleGETINFO(db, actor);
            }
        }

        return response;
    }

    static QString handleFriendADD(QSqlDatabase &db, const QString &username, const QString &friendname) {
        QSqlQuery query(db);
        query.prepare("SELECT 1 FROM client WHERE username=:username");
        query.bindValue(":username", friendname);
        if (!query.exec() || !query.next()) {
            return "未找到此人";
        }

        query.prepare("SELECT 1 FROM friend_relationship WHERE (user_id=:userid AND friend_id=:friendid) OR (user_id=:friendid AND friend_id=:userid)");
        query.bindValue(":userid", username);
        query.bindValue(":friendid", friendname);
        if (!query.exec()) {
            return "未知错误,添加失败";
        }
        if (query.next()) {
            return "用户已经是你的好友了";
        }

        query.prepare("INSERT INTO friend_relationship (user_id, friend_id, status) VALUES (:user_id, :friend_id, 0)");
        query.bindValue(":user_id", username);
        query.bindValue(":friend_id", friendname);
        if (!query.exec()) {
            qWarning() << "Insert failed:" << query.lastError().text();
            return "未知错误,添加失败";
        }
        return "已发送请求";
    }

    static QString handleFriendARGEE(QSqlDatabase &db, const QString &username, const QString &friendid) {
        QSqlQuery query(db);
        query.prepare("UPDATE friend_relationship SET status=1 WHERE friend_id=:friendid and user_id=:userid");
        query.bindValue(":friendid", username);
        query.bindValue(":userid", friendid);
        if (!query.exec()) {
            qWarning() << "改变status失败" << query.lastError().text();
            return "未知错误";
        }
        return "添加成功";
    }

    static QString handleFriendREFUSE(QSqlDatabase &db, const QString &username, const QString &friendid) {
        QSqlQuery query(db);
        query.prepare("DELETE FROM friend_relationship WHERE user_id=:friendid and friend_id=:userid");
        query.bindValue(":friendid", username);
        query.bindValue(":userid", friendid);
        if (!query.exec()) {
            qWarning() << "删除失败" << query.lastError().text();
            return "未知错误";
        }
        return "已拒绝";
    }

    static QStringList handleusersFRIENDNOW(QSqlDatabase &db, const QString &username) {
        QSqlQuery query(db);
        query.prepare("SELECT user_id,friend_id FROM friend_relationship WHERE (friend_id=:friend_id or user_id=:user_id) and status=1");
        query.bindValue(":friend_id", username);
        query.bindValue(":user_id", username);

        QSqlQuery queryONLINE(db);
        QStringList friendIds;
        auto readOnline = [&queryONLINE](const QString &name) -> QString {
            queryONLINE.prepare("SELECT is_online FROM client WHERE username=:username");
            queryONLINE.bindValue(":username", name);
            if (queryONLINE.exec() && queryONLINE.next()) {
                return queryONLINE.value(0).toString();
            }
            return QStringLiteral("0");
        };

        if (query.exec()) {
            while (query.next()) {
                if (query.value(0).toString() != username) {
                    friendIds.append(query.value(0).toString() + "+" + readOnline(query.value(0).toString()));
                }
                if (query.value(1).toString() != username) {
                    friendIds.append(query.value(1).toString() + "+" + readOnline(query.value(1).toString()));
                }
            }
        }
        return friendIds;
    }

    static QStringList handleusersDisplayFRIEND(QSqlDatabase &db, const QString &username) {
        QSqlQuery query(db);
        query.prepare("SELECT user_id FROM friend_relationship WHERE friend_id=:friend_id and status=0");
        query.bindValue(":friend_id", username);
        QStringList userIds;
        if (query.exec()) {
            while (query.next()) {
                userIds.append(query.value(0).toString());
            }
        }
        return userIds;
    }

    static QString handleGETINFO(QSqlDatabase &db, const QString &friendid) {
        QSqlQuery query(db);
        query.prepare("SELECT ip,port FROM client WHERE username=:friendid");
        query.bindValue(":friendid", friendid);

        QString reply;
        if (query.exec()) {
            while (query.next()) {
                reply.append(query.value(0).toString());
                reply.append("::");
                reply.append(query.value(1).toString());
            }
        }
        return reply;
    }
};


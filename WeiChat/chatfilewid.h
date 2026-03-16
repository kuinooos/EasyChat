#ifndef CHATFILEWID_H
#define CHATFILEWID_H

#include <QWidget>

namespace Ui {
class ChatFileWid;
}

class ChatFileWid : public QWidget
{
    Q_OBJECT

public:
    enum Mode { Sender, Receiver };

    explicit ChatFileWid(Mode mode, const QString &fileName, qint64 fileSize, const QString &peerName, QWidget *parent = nullptr);
    ~ChatFileWid();

    void updateProgress(qint64 current, qint64 total);
    void setCompleted();
    void setFailed();

    QString fileName() const { return m_fileName; }
    qint64 fileSize() const { return m_fileSize; }
    QString peerName() const { return m_peerName; }

signals:
    void sig_downloadClicked(const QString &fileName, qint64 fileSize, const QString &peerName);

private slots:
    void on_download_btn_clicked();

private:
    Ui::ChatFileWid *ui;
    Mode m_mode;
    QString m_fileName;
    qint64 m_fileSize;
    QString m_peerName;
    bool m_isCompleted = false;

    QString formatSize(qint64 size);
};

#endif // CHATFILEWID_H

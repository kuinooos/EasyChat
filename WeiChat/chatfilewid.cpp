#include "chatfilewid.h"
#include "ui_chatfilewid.h"
#include <QFileInfo>

ChatFileWid::ChatFileWid(Mode mode, const QString &fileName, qint64 fileSize, const QString &peerName, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatFileWid),
    m_mode(mode),
    m_fileName(fileName),
    m_fileSize(fileSize),
    m_peerName(peerName)
{
    ui->setupUi(this);
    ui->file_name_lb->setText(fileName);
    ui->file_size_lb->setText(formatSize(fileSize));
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->ok_lb->hide();

    if (m_mode == Sender) {
        ui->download_btn->hide();
        ui->progressBar->show();
    } else {
        ui->download_btn->show();
        ui->progressBar->hide();
    }
}

ChatFileWid::~ChatFileWid()
{
    delete ui;
}

void ChatFileWid::updateProgress(qint64 current, qint64 total)
{
    if (total <= 0) return;
    int progress = static_cast<int>((current * 100) / total);
    ui->progressBar->setValue(progress);
    ui->progressBar->show(); // Ensure visible during transfer
    
    if (m_mode == Receiver) {
        ui->download_btn->hide();
    }
}

void ChatFileWid::setCompleted()
{
    m_isCompleted = true;
    ui->progressBar->hide();
    ui->ok_lb->show();
    if (m_mode == Receiver) {
        ui->download_btn->hide();
    }
}

void ChatFileWid::setFailed()
{
    ui->ok_lb->setText(QStringLiteral("✖"));
    ui->ok_lb->setStyleSheet("color: red;");
    ui->ok_lb->show();
    ui->progressBar->hide();
}

void ChatFileWid::on_download_btn_clicked()
{
    emit sig_downloadClicked(m_fileName, m_fileSize, m_peerName);
}

QString ChatFileWid::formatSize(qint64 size)
{
    double s = static_cast<double>(size);
    QString unit = "B";
    if (s > 1024) { s /= 1024; unit = "KB"; }
    if (s > 1024) { s /= 1024; unit = "MB"; }
    if (s > 1024) { s /= 1024; unit = "GB"; }
    return QString::number(s, 'f', 2) + " " + unit;
}

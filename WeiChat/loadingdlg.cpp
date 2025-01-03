#include "loadingdlg.h"
#include "ui_loadingdlg.h"

LoadingDlg::LoadingDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoadingDlg)
{
    ui->setupUi(this);
}

LoadingDlg::~LoadingDlg()
{
    delete ui;
}

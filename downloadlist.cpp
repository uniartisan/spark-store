#include "downloadlist.h"
#include "ui_downloadlist.h"
#include <QDebug>
#include <QIcon>
#include <QPixmap>
downloadlist::downloadlist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::downloadlist)
{
    ui->setupUi(this);
    ui->pushButton->setEnabled(false);
    ui->progressBar->setValue(0);
    ui->label_filename->hide();
    ui->label->setStyleSheet("color:#000000");
}

downloadlist::~downloadlist()
{
    delete ui;
}

void downloadlist::setValue(long long value)
{
    ui->progressBar->setValue(value);
    ui->label_2->setText(QString::number((double)value/100)+"%");
    if(ui->label_2->text()=="100%"){
        ui->label_2->setText("已完成，等待安装");
    }
}

void downloadlist::setMax(long long max)
{
    ui->progressBar->setMaximum(max);
}

void downloadlist::setName(QString name)
{
    ui->label->setText(name);
}

QString downloadlist::getName()
{
    return ui->label_filename->text();
}

void downloadlist::readyInstall()
{
    ui->progressBar->hide();
    ui->pushButton->setEnabled(true);
    system("notify-send \""+ui->label->text().toUtf8()+"下载完成，等待安装\"");
}

void downloadlist::choose(bool isChoosed)
{
    if(isChoosed){
        ui->label->setStyleSheet("color:#FFFFFF");
        ui->label_2->setStyleSheet("color:#FFFFFF");
    }else {
        ui->label->setStyleSheet("color:#000000");
        ui->label_2->setStyleSheet("color:#000000");
    }
}

void downloadlist::setFileName(QString fileName)
{
    ui->label_filename->setText(fileName);
}

void downloadlist::seticon(const QPixmap icon)
{
    ui->label_3->setPixmap(icon);
}

void downloadlist::on_pushButton_clicked()
{
    system("x-terminal-emulator -e sudo apt install -y ./"+ui->label_filename->text().toUtf8());
    qDebug()<<ui->label_filename->text().toUtf8();
}

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QSpacerItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString strTitle = QString("简易标签打印 - V1.15 (Build: %1) - by QT%2 [正式版]").arg(__TIMESTAMP__, QT_VERSION_STR) ;
    setWindowTitle( strTitle );
    QTimer::singleShot(200,this,[=]{ ui->frameR->BindLabelView(ui->frameL) ;});

    QString strText("使用方法和技巧：单击控件，按上下左右键可移动，＋－键可放大缩小，同时按住Ctrl键可整体控制全部控件；Delete键删除控件。Ctrl+S：快速保存模板！") ;
    QLabel *pLabelTip = new QLabel(strText,this) ;
    pLabelTip->setStyleSheet("QLabel{ padding-left:10px; color:blue; }") ;
    ui->statusbar->addWidget(pLabelTip) ;

    QLabel *pLabelTime = new QLabel(strText,this) ;
    pLabelTime->setStyleSheet("QLabel{padding-left:10px;color:blue; font-weight: bold;}") ;
    ui->statusbar->addWidget(pLabelTime) ;

    QTimer *pTimerFlash = new QTimer(this) ;
    connect(pTimerFlash,&QTimer::timeout,this,[=]{
        static int nCount = 0 ;
        pLabelTip->setStyleSheet(QString("QLabel{padding-left:10px;color:%1;}").arg(nCount++ %2 ? "blue":"black") );
        pLabelTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")) ;
    }) ;

    pTimerFlash->start(1000) ;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(QMessageBox::question(this,"提示","确定要退出标签打印程序吗？") != QMessageBox::Yes)
    {
        event->ignore() ;
        return ;
    }
    ui->frameL->Save() ;
    QMainWindow::closeEvent(event) ;

}

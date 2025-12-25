#include "VideoAndPtzWidget.h"
#include "ui_VideoAndPtzWidget.h"

#include <QMessageBox>
#include <QResizeEvent>
#include <QTimer>
#include <QPainter>
#include <QDateTime>
#include <MainWidget.h>

VideoAndPtzWidget::VideoAndPtzWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::VideoAndPtzWidget ), m_bVideoStop( true ), m_cur_img_w( -1 ), m_cur_img_h( -1 )
{
    ui->setupUi( this );

    ui->label_video->installEventFilter(this) ;
}

VideoAndPtzWidget::~VideoAndPtzWidget()
{
    delete ui;
}

bool VideoAndPtzWidget::eventFilter(QObject *target, QEvent *event)
{
    if(target == ui->label_video && event->type() == QEvent::Paint)
    {
        if(!m_imgFrame.isNull())
        {
            QPainter painter(ui->label_video);
            imgLock();
            painter.drawImage(ui->label_video->rect(),m_imgFrame);
            imgUnlock();
            return true ;
        }
    }
    return QWidget::eventFilter(target, event) ;
}


void VideoAndPtzWidget::showVideoFrame(const QImage&img)
{
    imgLock();
    m_imgFrame = img ;
    m_bImgSet = true ;

    QPainter painter(&m_imgFrame);

    QPen pen(Qt::white);
    painter.setPen(pen);
    QFont font("Arial", 32);
    painter.setFont(font);

    QDateTime now = QDateTime::currentDateTime();
    QString text = now.toString("yyyy-MM-dd hh:mm:ss.zzzz");
    QPoint textPosition(10, 40);
    painter.drawText(textPosition, text);

    painter.end();

    imgUnlock();

    ui->label_video->update() ;
}


void VideoAndPtzWidget::startVideo(const QString& strRtmpUrl)
{
    if(strRtmpUrl.isEmpty())
        return ;

    int nWait = 100;
    if(m_pRtmp)
    {
        m_pRtmp->stopVideo();
        nWait = 3000 ;
    }

    QTimer::singleShot(nWait,this,[=]{
        if(m_pRtmp) delete m_pRtmp ;
        m_pRtmp = new CRtmpVideoHandle(this);

        qDebug() << "starting video..." << strRtmpUrl;
        connect( m_pRtmp, &CRtmpVideoHandle::sigVideoLog,this, [=](const QString&strLog) {
            emit onVideoLog(strLog) ;
        });

        connect( m_pRtmp, &CRtmpVideoHandle::sigRtmpStopped,this, [=](CRtmpVideoHandle *pOld) {
            qDebug() << "Rtmp stopped... " << pOld;
            stopVideo() ;
            QTimer::singleShot(2000,this,[=]{ clear(); });
            //pOld->deleteLater() ;
        });

        connect(m_pRtmp, &CRtmpVideoHandle::sigVideoOnline,this,[this]{ emit onVideoOnline();});
        connect(m_pRtmp, &CRtmpVideoHandle::sigImgFrame,this,[this](const QImage&img){
            showVideoFrame(img) ;
        });

        m_strUrl = strRtmpUrl;
        m_pRtmp->startVideo(strRtmpUrl);
        ui->lineEditUrl->setText(strRtmpUrl);

        m_bVideoStop = false;
        ui->pbtn_start_stop_video->setText( "停止视频流/对焦" );
        setStatusText( "视频启动成功" );
    });

}

void VideoAndPtzWidget::stopVideo()
{
    if(!m_bVideoStop && m_pRtmp)
    {
        qDebug() << "stopping video...";
        m_pRtmp->stopVideo();
    }
    m_bVideoStop = true;
    clear() ;
    ui->pbtn_start_stop_video->setText( "启动视频流/对焦" );
    setStatusText( "视频停止成功" );
}

void VideoAndPtzWidget::on_pbtn_ptz_up_clicked()
{
    emit onPZTCtrol(0);
    setStatusText( "云台向上成功" );
}

void VideoAndPtzWidget::on_pbtn_ptz_down_clicked()
{
    emit onPZTCtrol(1);
    setStatusText( "云台向下成功" );
}

void VideoAndPtzWidget::on_pbtn_ptz_left_clicked()
{
    emit onPZTCtrol(2);
    setStatusText( "云台向左成功" );
}

void VideoAndPtzWidget::on_pbtn_ptz_right_clicked()
{
    emit onPZTCtrol(3);
    setStatusText( "云台向右成功" );
}

void VideoAndPtzWidget::on_pbtn_ptz_stop_clicked()
{
    emit onPZTCtrol(4);
    setStatusText( "云台停止成功" );
}

void VideoAndPtzWidget::on_pbtn_ptz_reset_clicked()
{
    emit onPZTCtrol(5);
    setStatusText( "云台复位成功" );
}

void VideoAndPtzWidget::on_pbtn_start_stop_video_clicked()
{
    if( m_bVideoStop )
    {
        qDebug() << "Starting video...";
        startVideo(m_strUrl);
    }
    else
    {
        qDebug() << "Stopping video...";
        stopVideo();
    }
}

void VideoAndPtzWidget::setStatusText( const QString &status_text )
{
    ui->label_status->setText( status_text );
}

void VideoAndPtzWidget::clear()
{
    ui->label_video->clear() ;
    ui->label_video->setText("视频区域") ;
}


void VideoAndPtzWidget::on_checkBoxAutoPlay_clicked()
{
    if(!m_bVideoStop)
    {
        emit onVideoMannual() ;
        stopVideo();
    }

    if(ui->checkBoxAutoPlay->isChecked())
        startVideo(ui->lineEditUrl->text().trimmed()) ;
}


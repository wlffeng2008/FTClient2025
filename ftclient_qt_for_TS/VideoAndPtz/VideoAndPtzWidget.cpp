#include "VideoAndPtzWidget.h"
#include "ui_VideoAndPtzWidget.h"

#include <QMessageBox>
#include <QPainter>
#include <QDateTime>

VideoAndPtzWidget::VideoAndPtzWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::VideoAndPtzWidget ), m_bVideoStop( true ), m_cur_img_w( -1 ), m_cur_img_h( -1 )
{
    ui->setupUi( this );

    // 设置 style 属性，用于应用定制风格
    this->setProperty( "PanelStyle", true );

    //ui->labelUrl->hide() ;
    //ui->lineEditUrl->hide() ;
    //ui->pushButtonPlay->hide() ;
}

VideoAndPtzWidget::~VideoAndPtzWidget()
{
    delete ui;
}

void VideoAndPtzWidget::startVideo(const QString& strRtmpUrl)
{
    qDebug() << "starting video...";

    m_pRtmp = new CRtmpVideoHandle(this);

    connect( m_pRtmp, &CRtmpVideoHandle::sigVideoLog, this, [=](const QString&strLog) {
        emit onVideoLog(strLog) ;
    });

    connect( m_pRtmp, &CRtmpVideoHandle::sigRtmpStopped, this, [=](CRtmpVideoHandle *pOld) {
        qDebug() << "Rtmp stopped...";
        //pOld->deleteLater() ;
    });

    connect( m_pRtmp, &CRtmpVideoHandle::sigImgSize, this, [=](int w_img, int h_img){

        int margin = 3;
        int max_img_h = this->height() - margin*2;
        int max_img_w = this->width() - margin*2;

        if( m_cur_img_h <= 0 )
        {
            m_cur_img_h = max_img_h;
        }
        if( m_cur_img_w <= 0 )
        {
            m_cur_img_w = max_img_w;
        }

        m_cur_img_w = ( w_img <= 0 ) ? m_cur_img_w : w_img;
        m_cur_img_h = ( h_img <= 0 ) ? m_cur_img_h : h_img;

        updateLabelSize();
    });

    connect(m_pRtmp, &CRtmpVideoHandle::sigVideoOnline,this, [=]{ emit onVideoOnline();});
    connect(m_pRtmp, &CRtmpVideoHandle::sigImgFrame,this, [=](const QImage&img){
        showVideoFrame(img) ;
    });

    m_strUrl = strRtmpUrl;
    m_pRtmp->startVideo(strRtmpUrl);
    ui->lineEditUrl->setText(strRtmpUrl);

    m_bVideoStop = false;
    ui->pbtn_start_stop_video->setText( "停止视频流/对焦" );
    setStatusText( "视频启动成功" );
}

void VideoAndPtzWidget::stopVideo()
{
    if(m_bVideoStop)
        return ;
    qDebug() << "stopping video...";
    m_bVideoStop = true;
    if(m_pRtmp)
    {
        m_pRtmp->stopVideo();
        m_pRtmp = nullptr ;
    }
    clear() ;
    ui->pbtn_start_stop_video->setText( "启动视频流/对焦" );
    setStatusText( "视频已停止" );
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
    emit onPZTCtrol(4); setStatusText( "云台停止成功" );
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

void VideoAndPtzWidget::resizeEvent(QResizeEvent *event)
{
    updateLabelSize() ;
}

void VideoAndPtzWidget::updateLabelSize()
{
    int margin = 1;
    int max_img_w = ui->widget_video->width() - margin*2;
    int max_img_h = ui->widget_video->height() - margin*2 ;

    imgLock();

    if( m_cur_img_h <= 0 )
        m_cur_img_h = max_img_h;
    if( m_cur_img_w <= 0 )
        m_cur_img_w = max_img_w;

    float scale_w = max_img_w*1.0 / m_cur_img_w;
    float scale_h = max_img_h*1.0 / m_cur_img_h;
    float scale_min = std::min(scale_w,scale_h);

    int w_aim = max_img_w ;
    int h_aim = max_img_w * ( m_cur_img_h*1.0/m_cur_img_w);

    if(h_aim > max_img_h - 200)
    {
        h_aim = max_img_h - 200;
    }

    if(h_aim > w_aim)
    {
        h_aim = w_aim * 9.0/16;
    }

    ui->label_video->setFixedSize(w_aim-16, h_aim );

    qDebug() << w_aim << h_aim;

    imgUnlock();
}

void VideoAndPtzWidget::setStatusText( const QString &status_text )
{
    ui->label_status->setText( status_text );
}

void VideoAndPtzWidget::showVideoFrame(const QImage&img)
{
    if(img.isNull())
    {
        clear() ;
        return ;
    }

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

    QPixmap PMap = QPixmap::fromImage(m_imgFrame);
    if(!PMap.isNull())
        ui->label_video->setPixmap( PMap.copy());

    imgUnlock();
}

void VideoAndPtzWidget::clear()
{
    ui->label_video->setText("视频区域") ;
}

void VideoAndPtzWidget::on_checkBoxAutoPlay_clicked()
{
    emit onVideoMannual() ;
    stopVideo();
    if(ui->checkBoxAutoPlay->isChecked())
    {
        startVideo(ui->lineEditUrl->text().trimmed()) ;
    }
}


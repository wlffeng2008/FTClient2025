#include "VideoAndPtzWidget.h"
#include "ui_VideoAndPtzWidget.h"
#include "CSysSettings.h"
#include <QMessageBox>
#include <QResizeEvent>
#include <QTimer>

VideoAndPtzWidget::VideoAndPtzWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::VideoAndPtzWidget ), m_bVideoStop( true ), m_cur_img_w( -1 ), m_cur_img_h( -1 )
{
    ui->setupUi( this );

    // 设置 style 属性，用于应用定制风格
     this->setProperty( "PanelStyle", true );

    connect(&m_TMShowImg,&QTimer::timeout,this,[this](){
         if( m_bImgSet )
         {
             QPixmap img_p = QPixmap::fromImage(m_imgFrame);
             if(!img_p.isNull())
                 ui->label_video->setPixmap( img_p );
         }
          m_bImgSet=false ;
     });

     m_TMShowImg.start(20) ;
}

VideoAndPtzWidget::~VideoAndPtzWidget()
{
    delete ui;
}

void VideoAndPtzWidget::setDeviceId(const QString &deviceId)
{
    m_curDeviceId = deviceId;
    getRtmpVideoHandle()->setCurDeviceId( deviceId);
}

std::shared_ptr<CRtmpVideoHandle> VideoAndPtzWidget::getRtmpVideoHandle()
{
    if( nullptr == m_pRtmpVideoHandle )
    {
        int rtmp_port = 1953;
        std::string rtmp_server_ip;
        if( !CSysSettings::getInstance()->getRtmpPara( rtmp_server_ip, rtmp_port ) )
        {
            qWarning() << "Get tcp para error";
        }

        std::string dev_type = CSysSettings::getInstance()->getCurDeviceType();
        QString url_root = QString("rtmp://%1:%2/live/").arg( rtmp_server_ip.data() ).arg( rtmp_port );
        qDebug() << "rtmp url root: "<< url_root;

        m_pRtmpVideoHandle = std::make_shared<CRtmpVideoHandle>( url_root, ui->label_video );
        m_pRtmpVideoHandle->setImageLabelWgtParent( this );

        connect( m_pRtmpVideoHandle.get(), &CRtmpVideoHandle::sigRtmpStopped, [=]() {
            qDebug() << "Rtmp stopped...";
            m_bVideoStop = true;
            if( ui != nullptr ){
                ui->pbtn_start_stop_video->setText( "启动视频流/对焦" );
            }
            setStatusText( "视频已停止" );
        });

        connect( m_pRtmpVideoHandle.get(), &CRtmpVideoHandle::sigImgSize, [=](int w_img, int h_img){

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

        connect(m_pRtmpVideoHandle.get(), &CRtmpVideoHandle::sigVideoOnline,[this]{ emit onVideoOnline();});
    }
    return m_pRtmpVideoHandle;
}

void VideoAndPtzWidget::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED(event) ;
    if(m_curDeviceId.isEmpty())
        return ;
    updateLabelSize();
}

void VideoAndPtzWidget::startVideo()
{
    this->startVideoPull();

    m_bVideoStop = false;
    ui->pbtn_start_stop_video->setText( "停止视频流/对焦" );
    setStatusText( "视频启动成功" );
}

void VideoAndPtzWidget::stopVideo()
{
    this->stopVideoPull();

    m_bVideoStop = true;
    clear() ;
    ui->pbtn_start_stop_video->setText( "启动视频流/对焦" );
    setStatusText( "视频停止成功" );
}

void VideoAndPtzWidget::startVideoPull()
{
    qDebug() << "starting video...";

    getRtmpVideoHandle()->setCurDeviceId( m_curDeviceId );
    getRtmpVideoHandle()->startVideo();
}

void VideoAndPtzWidget::stopVideoPull()
{
    getRtmpVideoHandle()->stopVideo();
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

void VideoAndPtzWidget::on_pbtn_start_stop_video_clicked()
{
    if( m_bVideoStop )
    {
        qDebug() << "Starting video...";
        startVideo();
    }
    else
    {
        qDebug() << "Stopping video...";
        stopVideo();
    }
}

void VideoAndPtzWidget::updateLabelSize()
{
    int margin = 1;
    int max_img_h = ui->widget_video->height() - margin*2;
    int max_img_w = ui->widget_video->width() - margin*2;

    imgLock();
    if( m_cur_img_h <= 0 )
        m_cur_img_h = max_img_h;
    if( m_cur_img_w <= 0 )
        m_cur_img_w = max_img_w;

    float scale_w = (float)max_img_w / (float)m_cur_img_w;
    float scale_h = (float)max_img_h / (float)m_cur_img_h;
    float scale_min = std::min( scale_h, scale_w );

    int w_aim = m_cur_img_w * scale_min;
    int h_aim = m_cur_img_h * scale_min;
    ui->label_video->setFixedSize( w_aim-16, h_aim );
    imgUnlock();
}

void VideoAndPtzWidget::setStatusText( const QString &status_text )
{
    ui->label_status->setText( status_text );
}

void VideoAndPtzWidget::showVideoFrame(const QImage&img)
{
    m_imgFrame = img ;
    m_bImgSet = true ;
}

void VideoAndPtzWidget::clear()
{
    ui->label_video->setText("视频区域") ;
}


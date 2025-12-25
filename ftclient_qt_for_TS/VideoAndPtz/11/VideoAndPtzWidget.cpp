#include "VideoAndPtzWidget.h"
#include "ui_VideoAndPtzWidget.h"
#include "CSysSettings.h"
#include <QMessageBox>
#include <QResizeEvent>
#include <QPainter>
#include <QTimer>

VideoAndPtzWidget::VideoAndPtzWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::VideoAndPtzWidget ), m_bVideoStop( true ), m_cur_img_w( -1 ), m_cur_img_h( -1 )
{
    ui->setupUi( this );

    // 设置 style 属性，用于应用定制风格
    this->setProperty( "PanelStyle", true );
}

VideoAndPtzWidget::~VideoAndPtzWidget()
{
    delete ui;
}

void VideoAndPtzWidget::setDeviceId(const QString &deviceId)
{
    m_curDeviceId = deviceId;
    getRtmpVideoHandle()->setCurDeviceId( m_curDeviceId.toStdString() );
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

        QObject::connect( m_pRtmpVideoHandle.get(), &CRtmpVideoHandle::errorOccurred, [=](const QString& error) {
            qWarning() << "Stream error: " << error;
            setStatusText( error );
        });

        QObject::connect( m_pRtmpVideoHandle.get(), &CRtmpVideoHandle::sigRtmpStopped, [=]() {
            qDebug() << "Rtmp stopped..";
            m_bVideoStop = true;
            if( ui != nullptr ){
                ui->pbtn_start_stop_video->setText( "启动视频流/对焦" );
            }
            setStatusText( "视频已停止" );
        });

        QObject::connect( m_pRtmpVideoHandle.get(), &CRtmpVideoHandle::sigImgSize, [=](int w_img, int h_img){

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
    }
    return m_pRtmpVideoHandle;
}

void VideoAndPtzWidget::releaseRtmpVideoHandle()
{
    m_pRtmpVideoHandle.reset();
    m_pRtmpVideoHandle = nullptr;
}

std::shared_ptr<CTcpVideoHandle> VideoAndPtzWidget::getTcpVideoHandle()
{
    if( nullptr == m_pTcpVideoHandle )
    {
        m_pTcpVideoHandle = std::make_shared<CTcpVideoHandle>();
        m_pTcpVideoHandle->setImageLabel( ui->label_video );
        m_pTcpVideoHandle->setImageLabelParent( this );

        QObject::connect( m_pTcpVideoHandle.get(), &CTcpVideoHandle::sigRepaint, [this]() {
            this->repaint();
        });
    }
    return m_pTcpVideoHandle;
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
    if(m_curDeviceId.isEmpty())
    {
        QMessageBox::warning(nullptr,"提示","设备未绑定！");
        return ;
    }
    // 启动读取视频
    this->startVideoPull();

    m_bVideoStop = false;
    ui->pbtn_start_stop_video->setText( "停止视频流/对焦" );
    setStatusText( "视频启动成功" );
}

void VideoAndPtzWidget::stopVideo()
{
    if(m_curDeviceId.isEmpty())
    {
        //QMessageBox::warning(nullptr,"提示","设备未绑定！");
        return ;
    }
    // 停止读取视频
    this->stopVideoPull();

    m_bVideoStop = true;
    ui->pbtn_start_stop_video->setText( "启动视频流/对焦" );
    setStatusText( "视频停止成功" );
}

void VideoAndPtzWidget::startVideoPull()
{
    qDebug() << "starting video..";
    std::string cur_dev_com = CSysSettings::getInstance()->getCurDeviceComType();
    if( "4G" == cur_dev_com )
    {
        getRtmpVideoHandle()->setCurDeviceId( m_curDeviceId.toStdString() );
        getRtmpVideoHandle()->startVideo();
    }
    else
    {
        getTcpVideoHandle()->startVideo();
    }
}

void VideoAndPtzWidget::stopVideoPull()
{
    qDebug() << "stopping video..";
    std::string cur_dev_com = CSysSettings::getInstance()->getCurDeviceComType();
    if( "4G" == cur_dev_com )
    {
        getRtmpVideoHandle()->stopVideo();
    }
    else
    {
        getTcpVideoHandle()->stopVideo();
    }
}

void VideoAndPtzWidget::sendPZDCmd(int nCmd)
{
    if(m_curDeviceId.isEmpty())
    {
        QMessageBox::warning(nullptr,"提示","设备未绑定！");
        return ;
    }

    static QStringList A = {"向上","向下","向左","向右","停止","复位","未知命令"} ;
    QString strInfo = QString("云台 %1 成功！").arg(A[nCmd]) ;
    setStatusText(strInfo) ;

    emit onPZTCtrol(nCmd);
}

void VideoAndPtzWidget::on_pbtn_ptz_up_clicked()
{
    qDebug() << "on_pbtn_ptz_up_clicked..";
    sendPZDCmd(0);
}

void VideoAndPtzWidget::on_pbtn_ptz_down_clicked()
{
    qDebug() << "on_pbtn_ptz_down_clicked..";
    sendPZDCmd(1);
}

void VideoAndPtzWidget::on_pbtn_ptz_left_clicked()
{
    qDebug() << "on_pbtn_ptz_left_clicked..";
    sendPZDCmd(2);
}

void VideoAndPtzWidget::on_pbtn_ptz_right_clicked()
{
    qDebug() << "on_pbtn_ptz_right_clicked..";
    sendPZDCmd(3);
}

void VideoAndPtzWidget::on_pbtn_ptz_stop_clicked()
{
    qDebug() << "on_pbtn_ptz_stop_clicked..";
    sendPZDCmd(4);
}

void VideoAndPtzWidget::on_pbtn_ptz_reset_clicked()
{
    qDebug() << "on_pbtn_ptz_reset_clicked..";
    sendPZDCmd(5);
}

void VideoAndPtzWidget::on_pbtn_start_stop_video_clicked()
{
    qDebug() << "on_pbtn_start_stop_video_clicked..m_bVideoStop: " << m_bVideoStop << ", btn_text: " << ui->pbtn_start_stop_video->text();

    if( m_bVideoStop )
    {
        qDebug() << "Starting video..";
        startVideo();
    }
    else
    {
        qDebug() << "Stopping video..";
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
    ui->label_video->setFixedSize( w_aim-10, h_aim );
    imgUnlock();
}

void VideoAndPtzWidget::setStatusText( const QString &status_text )
{
    if( ui != nullptr )
    {
        ui->label_status->setText( status_text );
    }
}

void VideoAndPtzWidget::showVideoFrame(const QImage&img)
{
    m_imgFrame = img ;
    update() ;
}

void VideoAndPtzWidget::paintEvent(QPaintEvent *e)
{
    if(!m_imgFrame.isNull())
    {
        QPixmap img_p = QPixmap::fromImage(m_imgFrame);
        ui->label_video->setScaledContents(true) ;
        if(!img_p.isNull())
            ui->label_video->setPixmap( img_p );
    }
}

#ifndef VIDEOANDPTZWIDGET_H
#define VIDEOANDPTZWIDGET_H

#include <QWidget>
#include <QMutex>
#include <QPaintEvent>
#include <memory>
#include "CRtmpVideoHandle.h"
#include "CTcpVideoHandle.h"

namespace Ui {
class VideoAndPtzWidget;
}

class VideoAndPtzWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoAndPtzWidget(QWidget *parent = nullptr);
    virtual ~VideoAndPtzWidget();

    void setDeviceId( const QString& deviceId );
    void startVideo();
    void stopVideo();
    void imgLock() { m_mtx_img.lock();}
    void imgUnlock() { m_mtx_img.unlock(); }

    void showVideoFrame(const QImage&img);

private slots:
    void on_pbtn_ptz_up_clicked();
    void on_pbtn_ptz_down_clicked();
    void on_pbtn_ptz_left_clicked();
    void on_pbtn_ptz_right_clicked();
    void on_pbtn_ptz_reset_clicked();
    void on_pbtn_ptz_stop_clicked();
    void on_pbtn_start_stop_video_clicked();

signals:
    void onPZTCtrol(int nDire) ;

private:
    void paintEvent(QPaintEvent *e) ;
    void startVideoPull();
    void stopVideoPull();
    bool sendVideoStartCmdToDevice();
    bool sendVideoStopCmdToDevice();
    void releaseRtmpVideoHandle();

    void setStatusText( const QString& status_text );
    std::shared_ptr<CRtmpVideoHandle> getRtmpVideoHandle();
    std::shared_ptr<CTcpVideoHandle> getTcpVideoHandle();
    void resizeEvent( QResizeEvent *event );
    void updateLabelSize();
    void sendPZDCmd(int nCmd);

private:
    Ui::VideoAndPtzWidget *ui;
    std::shared_ptr<CRtmpVideoHandle> m_pRtmpVideoHandle;
    std::shared_ptr<CTcpVideoHandle> m_pTcpVideoHandle;
    bool m_bVideoStop;
    QString m_curDeviceId;
    int m_cur_img_w;
    int m_cur_img_h;
    QMutex m_mtx_img;
    QImage m_imgFrame;
};

#endif // VIDEOANDPTZWIDGET_H

#ifndef VIDEOANDPTZWIDGET_H
#define VIDEOANDPTZWIDGET_H

#include <QWidget>
#include <QMutex>
#include <QTimer>
#include <memory>
#include "CRtmpVideoHandle.h"

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
    void imgLock()  { m_mtx_img.lock(); }
    void imgUnlock() { m_mtx_img.unlock(); }

    void showVideoFrame(const QImage&img);
    void clear() ;

private slots:
    void on_pbtn_ptz_up_clicked();
    void on_pbtn_ptz_down_clicked();
    void on_pbtn_ptz_left_clicked();
    void on_pbtn_ptz_right_clicked();
    void on_pbtn_ptz_stop_clicked();
    void on_pbtn_start_stop_video_clicked();

signals:
    void onPZTCtrol(int nDire) ;
    void onVideoOnline() ;

private:
    void startVideoPull();
    void stopVideoPull();
    bool sendVideoStartCmdToDevice();
    bool sendVideoStopCmdToDevice();

    void setStatusText( const QString& status_text );
    std::shared_ptr<CRtmpVideoHandle> getRtmpVideoHandle();
    void resizeEvent( QResizeEvent *event );
    void updateLabelSize();

private:
    Ui::VideoAndPtzWidget *ui;
    std::shared_ptr<CRtmpVideoHandle> m_pRtmpVideoHandle;
    bool m_bVideoStop;
    QString m_curDeviceId;
    int m_cur_img_w;
    int m_cur_img_h;
    QMutex m_mtx_img;
    QImage m_imgFrame;
    bool m_bImgSet=false ;
    QTimer m_TMShowImg;
};

#endif // VIDEOANDPTZWIDGET_H

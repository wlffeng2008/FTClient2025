#ifndef VIDEOANDPTZWIDGET_H
#define VIDEOANDPTZWIDGET_H

#include <QWidget>
#include <QMutex>
#include <QTimer>
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

    void startVideo( const QString& strRtmpUrl );
    void stopVideo();
    void imgLock() { m_mtx_img.lock(); }
    void imgUnlock() { m_mtx_img.unlock(); }
    void showVideoFrame(const QImage&img);    
    void clear();

private slots:
    void on_pbtn_ptz_up_clicked();
    void on_pbtn_ptz_down_clicked();
    void on_pbtn_ptz_left_clicked();
    void on_pbtn_ptz_right_clicked();
    void on_pbtn_ptz_stop_clicked();
    void on_pbtn_ptz_reset_clicked();
    void on_pbtn_start_stop_video_clicked();
    void on_checkBoxAutoPlay_clicked();

signals:
    void onPZTCtrol(int nDire) ;
    void onVideoLog(const QString& strLog) ;
    void onVideoOnline() ;
    void onVideoMannual() ;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    bool sendVideoStartCmdToDevice();
    bool sendVideoStopCmdToDevice();

    void setStatusText( const QString& status_text );
    void updateLabelSize();

private:
    Ui::VideoAndPtzWidget *ui;
    CRtmpVideoHandle *m_pRtmp = nullptr;
    bool m_bVideoStop;
    int m_cur_img_w=1920;
    int m_cur_img_h=1080;
    QMutex m_mtx_img;
    QImage m_imgFrame;
    bool m_bImgSet=false ;
    QTimer m_TMShowImg;
    QString m_strUrl ;
};

#endif // VIDEOANDPTZWIDGET_H

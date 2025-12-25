#ifndef C_RTMP_VIDEO_HANDLE_H
#define C_RTMP_VIDEO_HANDLE_H

#include <QString>
#include <QThread>
#include "caudioplayer.h"

class AVFrame;
class QLabel;
class VideoAndPtzWidget;


class CRtmpVideoHandle : public QThread
{
    Q_OBJECT

public:
    explicit CRtmpVideoHandle( const QString& url_root ,QObject* parent = nullptr );
    virtual ~CRtmpVideoHandle();

    void setCurDeviceId( const QString & curDevId ){m_curDevId = curDevId;}

    void startVideo();
    void stopVideo();
    void setImageLabelWgtParent( VideoAndPtzWidget* pImageLabelWgtParent )
    {
        m_pImageLabelWgtParent = pImageLabelWgtParent;
    }

signals:
    void sigRtmpStopped();
    void sigVideoOnline();
    void sigImgSize( int img_w, int img_h );

protected:
    void run() override;

private:
    void displayFrame( AVFrame* frame );

private:
    QString rtmp_url_root;
    QString m_curDevId;
    VideoAndPtzWidget* m_pImageLabelWgtParent;

    bool m_bRunning = false;

    CAudioPlayer AX ;
};


#endif // C_RTMP_VIDEO_HANDLE_H

#ifndef C_RTMP_VIDEO_HANDLE_H
#define C_RTMP_VIDEO_HANDLE_H

#include <QString>
#include <QThread>
#include "caudioplayer.h"

class AVFrame;

class CRtmpVideoHandle : public QThread
{
    Q_OBJECT

public:
    explicit CRtmpVideoHandle(QObject* parent );
    virtual ~CRtmpVideoHandle();

    void startVideo(const QString&strUrl);
    void stopVideo();

signals:
    void sigRtmpStopped(CRtmpVideoHandle *);
    void sigVideoOnline();
    void sigVideoLog( const QString&strLog );
    void sigImgSize( int img_w, int img_h );
    void sigImgFrame( const QImage&img );

protected:
    void run() override;

private:
    void displayFrame( AVFrame* frame );

private:

    bool m_bRunning = false;

    CAudioPlayer AX ;
    QString m_strUrl ;
};


#endif // C_RTMP_VIDEO_HANDLE_H

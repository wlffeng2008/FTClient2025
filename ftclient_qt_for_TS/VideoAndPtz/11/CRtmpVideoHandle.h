#ifndef C_RTMP_VIDEO_HANDLE_H
#define C_RTMP_VIDEO_HANDLE_H

#include <QString>
#include <QThread>

class AVFrame;
class QLabel;
class VideoAndPtzWidget;


class CRtmpVideoHandle : public QThread {
    Q_OBJECT

public:
    explicit CRtmpVideoHandle( const QString& url_root, QLabel* pLabel, QObject* parent = nullptr );
    virtual ~CRtmpVideoHandle();

    void setVideoUrlRoot( const QString& url_root ){
        rtmp_url_root = url_root;
    }
    void setCurDeviceId( const std::string& curDevId );
    QString getCurUrl(){
        return rtmp_url_root + m_curDevId.data();
    }

    void startVideo();
    void stopVideo();
    void setImageLabel( QLabel* pImageLabel )
    {
        m_pImgLabel = pImageLabel;
    }
    void setImageLabelWgtParent( VideoAndPtzWidget* pImageLabelWgtParent )
    {
        m_pImageLabelWgtParent = pImageLabelWgtParent;
    }

signals:
    void errorOccurred( const QString& error );
    void sigRtmpStopped();
    void sigImgSize( int img_w, int img_h );

protected:
    void run() override;

private:
    void saveVideoStream( const char* output_file );
    void displayFrame( AVFrame* frame );
    bool waitForExited( int maxWaitTimeMs );

private:
    QString rtmp_url_root;
    std::string m_curDevId;
    QLabel* m_pImgLabel;
    VideoAndPtzWidget* m_pImageLabelWgtParent;
    int m_iDelay_ms;

    std::atomic<bool> m_b_running;
    std::atomic<bool> m_b_exited;
};


#endif // C_RTMP_VIDEO_HANDLE_H

#ifndef CVIDEOHANDLE_H
#define CVIDEOHANDLE_H

#include <string>
#include <QString>
#include <QThread>
#include <QMutex>

class AVFrame;
class QLabel;


class CVideoHandle : public QThread {
    Q_OBJECT

public:
    explicit CVideoHandle( const QString& url, QLabel* pLabel, QObject* parent = nullptr );
    virtual ~CVideoHandle();

    void stop();
    void setImageLabel( QLabel* pImageLabel )
    {
        m_pImgLabel = pImageLabel;
    }

signals:
    void errorOccurred(const QString& error);

protected:
    void run() override;

private:
    void displayFrame( AVFrame* frame );

private:
    QString rtmp_url;
    QLabel* m_pImgLabel;
    bool stopThread;
    QMutex mutex;
};


#endif // CVIDEOHANDLE_H

#ifndef CTCPVIDEOHANDLE_H
#define CTCPVIDEOHANDLE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QImage>
#include <QMutex>

class QLabel;


class CTcpVideoHandle : public QObject
{
    Q_OBJECT

public:
    explicit CTcpVideoHandle( QObject *parent = nullptr );
    virtual ~CTcpVideoHandle();

    void setImageLabel( QLabel* pImageLabel )
    {
        m_pImgLabel = pImageLabel;
    }
    void setImageLabelParent( QWidget* pImgWgtParent )
    {
        m_pImgWgtParent = pImgWgtParent;
    }
    void startVideo()
    {
        m_b_running = true;
    }
    void stopVideo()
    {
        m_b_running = false;
    }

signals:
    void sigRepaint();

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void decodeH264Data();

private:
    QTcpServer *server;
    QLabel* m_pImgLabel;
    QWidget* m_pImgWgtParent;
    std::atomic<bool> m_b_running;

    QByteArray m_buffer; // 缓冲区用于存储接收到的数据
    QMutex m_mutex_buffer;
};

#endif // CTCPVIDEOHANDLE_H

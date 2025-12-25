#include "CTcpVideoHandle.h"
#include <QPixmap>
#include <QLabel>
#include <synchapi.h>
#include "CommonLib.h"

#ifdef RTMP
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/timestamp.h>
#ifdef __cplusplus
}
#endif
#endif


CTcpVideoHandle::CTcpVideoHandle( QObject *parent )
    : QObject{parent}, server( new QTcpServer(this) ), m_pImgLabel( nullptr ), m_pImgWgtParent( nullptr ), m_b_running( false )
{
    connect( server, &QTcpServer::newConnection, this, &CTcpVideoHandle::onNewConnection );
    server->listen( QHostAddress::Any, 3264 );         // 监听端口
}

CTcpVideoHandle::~CTcpVideoHandle()
{

}

void CTcpVideoHandle::onNewConnection()
{
    QTcpSocket *clientSocket = server->nextPendingConnection();
    connect( clientSocket, &QTcpSocket::readyRead, this, &CTcpVideoHandle::onReadyRead );
}

void CTcpVideoHandle::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>( sender() );
    QByteArray data = clientSocket->readAll();
    if( data.size() <= 0 )
    {
        return;
    }
    qDebug() << "Received data: " << data.size() << " bytes";

    // 将数据缓存到 m_buffer 中
    qDebug() << "m_buffer _1 data: " << data.size() << " bytes";
    m_mutex_buffer.lock();
    m_buffer.append(data);
    m_mutex_buffer.unlock();
    qDebug() << "m_buffer _2 data: " << data.size() << " bytes";

    decodeH264Data();

    // QByteArray startCode( 4, 0 );
    // startCode[3] = 1;

    // while (true)
    // {
    //     // 查找H.264帧的起始码
    //     int startCodeIndex = cb::findSubByteArrayPos( m_buffer, startCode, 0 );
    //     if( startCodeIndex == -1 )
    //     {
    //         // 如果没有找到起始码，等待更多数据
    //         break;
    //     }

    //     // 查找下一个起始码，作为当前帧的结束
    //     int nextStartCodeIndex = cb::findSubByteArrayPos( m_buffer, startCode, startCodeIndex+4 );
    //     if (nextStartCodeIndex == -1)
    //     {
    //         // 如果没有找到下一个起始码，说明当前帧数据还未接收完整
    //         break;
    //     }

    //     int thirdStartCodeIndex = cb::findSubByteArrayPos( m_buffer, startCode, nextStartCodeIndex+4 );
    //     int forthStartCodeIndex = cb::findSubByteArrayPos( m_buffer, startCode, thirdStartCodeIndex+4 );
    //     int buffer_size = data.size();
    //     int buffer_len = data.length();

    //     // 提取出一帧数据
    //     qDebug() << "data received len: " << m_buffer.size();
    //     QByteArray frameData = m_buffer.mid(startCodeIndex, nextStartCodeIndex - startCodeIndex);
    //     qDebug() << "frameData len: " << frameData.size();

    //     // 从缓冲区中移除已处理的数据
    //     m_buffer.remove(0, nextStartCodeIndex);

    //     // 处理解码并显示该帧
    //     decodeH264Data(frameData);
    // }
}

void CTcpVideoHandle::decodeH264Data()
{
    if( !m_b_running )
        return;

#ifdef RTMP

    static const AVCodec *codec = nullptr;
    static AVCodecContext *codecContext = nullptr;
    static AVFrame *frame = nullptr;
    static AVPacket *packet = nullptr;
    static struct SwsContext *swsCtx = nullptr;
    static int initialized = 0;

    if (!initialized)
    {
        avdevice_register_all();
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec)
        {
            qWarning() << "Codec not found";
            return;
        }
        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext)
        {
            qWarning() << "Could not allocate video codec context";
            return;
        }
        if (avcodec_open2(codecContext, codec, nullptr) < 0)
        {
            qWarning() << "Could not open codec";
            return;
        }
        frame = av_frame_alloc();
        packet = av_packet_alloc();
        initialized = 1;
    }

    // 将数据分割成多个包进行处理
    static QByteArray remainingData;
    m_mutex_buffer.lock();
    remainingData.append( m_buffer );
    m_mutex_buffer.unlock();

    QByteArray startCode( 4, 0 );
    startCode[3] = 1;
    //while( true )
    {
        // 查找 NAL 单元的起始码 (0x000001 or 0x00000001)
        int startCodePos = cb::findSubByteArrayPos( remainingData, startCode, 0 );
        if( startCodePos == -1 )
        {
            // 如果没有找到起始码，可能需要缓冲更多数据
            return;
        }

        // 处理找到的 NAL 单元
        QByteArray nalUnit = remainingData.mid( startCodePos );
        remainingData = remainingData.mid(startCodePos + nalUnit.size());

        // 如果数据包不是完整的 NAL 单元，继续接收
        if (nalUnit.size() < 4 || nalUnit[0] != '\x00' || nalUnit[1] != '\x00' || nalUnit[2] != '\x00'|| nalUnit[3] != '\x01')
        {
            return;
        }

        // 设置 AVPacket 数据
        av_packet_unref(packet);
        packet->data = reinterpret_cast<uint8_t *>(nalUnit.data());
        packet->size = nalUnit.size();

        if (avcodec_send_packet(codecContext, packet) < 0)
        {
            qWarning() << "Error sending packet for decoding";
            return;
        }

        try{
            while (avcodec_receive_frame(codecContext, frame) == 0)
            {
                if (!swsCtx)
                {
                    swsCtx = sws_getContext(
                        codecContext->width, codecContext->height, codecContext->pix_fmt,
                        codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                }

                uint8_t *rgbData[1];
                int rgbStride[1];
                QImage image(codecContext->width, codecContext->height, QImage::Format_RGB888);

                rgbData[0] = image.bits();
                rgbStride[0] = image.bytesPerLine();

                int result = sws_scale(swsCtx, frame->data, frame->linesize, 0, codecContext->height, rgbData, rgbStride);
                if (result <= 0) {
                    qWarning() << "sws_scale failed or returned no lines processed.";
                    continue;
                }

                if (m_b_running && (m_pImgLabel != nullptr))
                {
                    m_pImgLabel->setFixedSize(codecContext->width, codecContext->height);
                    QPixmap pix = QPixmap::fromImage(image);
                    m_pImgLabel->setPixmap(pix);
                    emit sigRepaint();
                    //m_b_running = false;
                }
            }
        } catch (const std::exception &e) {
            // 捕获标准异常并处理
            qDebug() << "Exception caught:" << e.what();
        } catch (...) {
            // 捕获所有类型的异常
            qDebug() << "Unknown exception caught!";
        }
    }

#endif
}


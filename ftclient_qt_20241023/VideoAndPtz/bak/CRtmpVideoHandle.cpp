#include "CRtmpVideoHandle.h"
#include "VideoAndPtzWidget.h"
#include "CSysSettings.h"

#include <QLabel>
#include <QImage>
#include <QDebug>
#include <QBuffer>
#include <QMessageBox>
#include <QCoreApplication>

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


CRtmpVideoHandle::CRtmpVideoHandle(const QString& url_root, QObject* parent )
    : QThread( parent ), rtmp_url_root( url_root ), m_pImageLabelWgtParent( nullptr )
{
   m_bRunning = false ;
   start();
}

CRtmpVideoHandle::~CRtmpVideoHandle()
{
    AX.forceExit();
}

void CRtmpVideoHandle::startVideo()
{
    qDebug() << "Rtmp thread starting...";
    m_bRunning = true ;
}

void CRtmpVideoHandle::stopVideo()
{
    qDebug() << "Rtmp thread stopping...";
    m_bRunning = false ;
    QThread::msleep(1000) ;
}

/* My G711 Encoder */
#define SIGN_BIT        (0x80)  /* Sign bit for a A-law byte. */
#define QUANT_MASK      (0xf) /* Quantization field mask. */
#define NSEGS           (8)        /* Number of A-law segments. */
#define SEG_SHIFT       (4)    /* Left shift for segment number. */
#define SEG_MASK        (0x70)  /* Segment field mask. */
#define BIAS            (0x84)      /* Bias for linear code. */

static int alaw2linear(unsigned char a_val)
{
    int t=0;
    int seg=0;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg) {
    case 0:
        t += 8;
        break;
    case 1:
        t += 0x108;
        break;
    default:
        t += 0x108;
        t <<= seg - 1;
    }

    int nSample = ((a_val & SIGN_BIT) ? t : -t) * 1.5 ;
    if (nSample > 32767) {
        nSample = 32767;
    } else if (nSample < -32768) {
        nSample = -32768;
    }

    return nSample;
}

void CRtmpVideoHandle::run()
{
    msleep( 2000 );

    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVCodecParameters* codecpar = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;

    avdevice_register_all();
    avformat_network_init();

    qDebug() << "avformat_network_init finished";

    while(true)
    {
        if(!m_bRunning)
        {
            QThread::msleep(10) ;
            continue;
        }

        while( m_bRunning )
        {
            format_ctx = avformat_alloc_context();
            qDebug() << "avformat_alloc_context finished";

            format_ctx->probesize = 1 * 1024 * 1024;
            format_ctx->max_analyze_duration = 5 * 1000 * 1000;

            AVDictionary *options = NULL;

            // 设置超时时间，单位为微秒
            av_dict_set(&options, "timeout", "10000", 0);

            QString strUrl = rtmp_url_root + m_curDevId;
            if(avformat_open_input(&format_ctx, strUrl.toStdString().c_str(), nullptr,nullptr)<0)
            {
                //QMessageBox::critical(nullptr,"提示","RTMP拉流失败（超时）！");
                continue;
            }
            qDebug() << "avformat_open_input finished: " << strUrl;

            avformat_find_stream_info(format_ctx, nullptr);
            qDebug() << "avformat_find_stream_info finished";

            int video_stream_index = -1;
            int audio_stream_index = -1;
            for (unsigned int i = 0; i < format_ctx->nb_streams; i++)
            {
                if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                    video_stream_index = i;

                if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                    audio_stream_index = i;
            }

            qDebug() << "video stream found: " << video_stream_index << audio_stream_index;

            codecpar = format_ctx->streams[video_stream_index]->codecpar;
            const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
            qDebug() << "avcodec_find_decoder finished";

            codec_ctx = avcodec_alloc_context3(codec);
            qDebug() << "avcodec_alloc_context3 finished";

            if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {  }
            qDebug() << "avcodec_parameters_to_context finished";

            if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {   }
            qDebug() << "avcodec_open2 finished";

            frame = av_frame_alloc();
            qDebug() << "av_frame_alloc finished";

            packet = av_packet_alloc();
            qDebug() << "av_packet_alloc finished";

            while (m_bRunning && av_read_frame(format_ctx, packet) >= 0)
            {
                emit sigVideoOnline() ;

                if (packet->stream_index == video_stream_index)
                {
                    if (avcodec_send_packet(codec_ctx, packet) == 0)
                    {
                        while(m_bRunning && avcodec_receive_frame(codec_ctx, frame) == 0)
                            displayFrame(frame);
                    }
                }

                if(audio_stream_index == packet->stream_index)
                {
                    // 解码G.711 A-law数据为PCM
                    QByteArray pcmData;
                    for (int i=0; i<packet->size; i++)
                    {
                        unsigned char byte = packet->data[i] ;
                        short pcmSample = alaw2linear(byte&0xFF);
                        pcmData.append(reinterpret_cast<const char*>(&pcmSample), sizeof(short));
                    }
                    AX.pushBuf(pcmData) ;
                }
                av_packet_unref(packet);
            }

            if(!m_bRunning)
                break;
        }
        m_bRunning = false ;
        m_curDevId.clear() ;

        if(frame)
            av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        qDebug() << "avformat_free_context finished..............";

        m_pImageLabelWgtParent->clear();

        emit sigRtmpStopped();
    }
}


void CRtmpVideoHandle::displayFrame( AVFrame* frame )
{
    static int width_last = 0;
    static int height_last = 0;
    int frame_width = frame->width;
    int frame_height = frame->height;
    if( frame_width != width_last || frame_height != height_last )
        emit sigImgSize( frame_width, frame_height );

    width_last = frame_width;
    height_last = frame_height;

    SwsContext* sws_ctx = sws_getContext(
        frame_width, frame_height, (AVPixelFormat)frame->format,
        frame_width, frame_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if ( !sws_ctx ) {
        qDebug() << "Failed to create SwsContext.";
        return;
    }

    // 为 RGB 图像分配空间
    static uint8_t* rgb_data[4]={nullptr};
    static int rgb_linesize[4]={0};
    if(rgb_linesize[0] == 0)
        av_image_alloc( rgb_data, rgb_linesize, frame_width, frame_height, AV_PIX_FMT_RGB24, 1 );

    if(frame->linesize[0]>0)
    {
        // 将帧转换为 RGB24 格式
        sws_scale( sws_ctx, frame->data, frame->linesize, 0, frame_height, rgb_data, rgb_linesize );

        QImage img( rgb_data[0], frame_width, frame_height, rgb_linesize[0], QImage::Format_RGB888 );
        if(!img.isNull())
            m_pImageLabelWgtParent->showVideoFrame(img) ;
    }

    //av_freep( rgb_data );
    sws_freeContext( sws_ctx );
}


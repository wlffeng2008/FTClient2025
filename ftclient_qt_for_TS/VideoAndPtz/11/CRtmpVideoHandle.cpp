#include "CRtmpVideoHandle.h"
#include "VideoAndPtzWidget.h"
#include "CSysSettings.h"
#include "caudioplayer.h"

#include <QLabel>
#include <QImage>
#include <QDebug>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioSink>
#include <QBuffer>
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


CRtmpVideoHandle::CRtmpVideoHandle(const QString& url_root, QLabel* pLabel, QObject* parent )
    : QThread( parent ), rtmp_url_root( url_root ), m_pImgLabel( pLabel ), m_pImageLabelWgtParent( nullptr ), m_iDelay_ms( 5000 )
{
    m_b_running.store( false );
    m_b_exited.store( true );
}

CRtmpVideoHandle::~CRtmpVideoHandle()
{
    stopVideo();
    waitForExited( 300 );
}

void CRtmpVideoHandle::setCurDeviceId( const std::string &curDevId )
{
    if( curDevId != m_curDevId )
        stopVideo();
    m_curDevId = curDevId;
}

void CRtmpVideoHandle::startVideo()
{
    qDebug() << "Rtmp thread starting..";
    if( m_b_running.load() )
    {
        qDebug() << "Rtmp thread has been running, no need to start again.";
        return;
    }

    m_b_running.store( true );
    int delay_ms = 1000;
    if( CSysSettings::getInstance()->getRtmpDelay_ms( delay_ms ) )
    {
        m_iDelay_ms = delay_ms;
    }

    start();
}

void CRtmpVideoHandle::stopVideo()
{
    qDebug() << "Rtmp thread stopping..";
    m_b_running.store( false );
    waitForExited( 200 );
    //quit();
}

// 等待变量变为true，最大等待时间为maxWaitTimeMs
bool CRtmpVideoHandle::waitForExited( int maxWaitTimeMs )
{
    int elapsed = 0;
    int interval = 10;  // 轮询间隔10ms
    while( elapsed < maxWaitTimeMs )
    {
        if( m_b_exited )
        {
            qDebug() << "m_b_exited true within time limit.";
            return true;
        }
        //std::this_thread::sleep_for( std::chrono::milliseconds( interval ) );  // 休眠一小段时间
        msleep( std::chrono::milliseconds( interval ).count() );
        elapsed += interval;
    }
    qDebug() << "Timeout reached before variable changed.";
    return false;
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


CAudioPlayer AX ;

void CRtmpVideoHandle::run()
{
    m_b_exited = false;
    msleep( m_iDelay_ms );

#ifdef RTMP

    avdevice_register_all();
    avformat_network_init();

    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVCodecParameters* codecpar = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;

    while( m_b_running.load() )
    {
        format_ctx = avformat_alloc_context();
        qDebug() << "avformat_alloc_context finished";

        format_ctx->probesize = 10 * 1024 * 1024;
        format_ctx->max_analyze_duration = 10 * 1000 * 1000;

        std::string url = rtmp_url_root.toStdString() + m_curDevId;
        if(avformat_open_input(&format_ctx, url.c_str(), nullptr, nullptr)<0)
            break;
        qDebug() << "avformat_open_input finished: " << url.c_str();

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


        while (av_read_frame(format_ctx, packet) == 0)
        {
            if (packet->stream_index == video_stream_index)
            {
                if (avcodec_send_packet(codec_ctx, packet) == 0 &&packet->size > 100)
                {
                    while(true)
                    {
                        int ret = avcodec_receive_frame(codec_ctx, frame);
                        if (ret != 0)
                            break;
                         displayFrame(frame);
                    } ;
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
    }

    qDebug() << "av_frame_free starting..";
    av_frame_free(&frame);
    qDebug() << "av_packet_free starting..";
    av_packet_free(&packet);
    qDebug() << "avcodec_free_context starting..";
    avcodec_free_context(&codec_ctx);
    qDebug() << "avformat_free_context starting..";
    avformat_free_context(format_ctx);
    qDebug() << "avformat_free_context finished.";

    m_b_running = false;
    m_b_exited = true;
    emit sigRtmpStopped();
#endif
}

void CRtmpVideoHandle::saveVideoStream(const char* output_file)
{
#ifdef RTMP

    avdevice_register_all();
    avformat_network_init();

    // Allocate and configure format context for output file
    AVFormatContext* out_format_ctx = avformat_alloc_context();

    const AVOutputFormat* output_format = av_guess_format(nullptr, output_file, nullptr);
    out_format_ctx->oformat = output_format;

    // Open output file
    avio_open(&out_format_ctx->pb, output_file, AVIO_FLAG_WRITE);

    // Create and add a new video stream to the output file
    AVStream* out_stream = avformat_new_stream(out_format_ctx, nullptr);
    out_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    out_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    out_stream->codecpar->width  = 1920; // Change as needed
    out_stream->codecpar->height = 1080; // Change as needed
    out_stream->codecpar->format = AV_PIX_FMT_YUV420P;

    // Find encoder and open codec
    const AVCodec* encoder = avcodec_find_encoder(out_stream->codecpar->codec_id);
    AVCodecContext* encoder_ctx = avcodec_alloc_context3(encoder);

    avcodec_parameters_to_context(encoder_ctx, out_stream->codecpar);

    avcodec_open2(encoder_ctx, encoder, nullptr) ;

    avformat_write_header(out_format_ctx, nullptr);

    AVFormatContext* format_ctx = avformat_alloc_context();
    QString url = rtmp_url_root + m_curDevId.data();
    avformat_open_input(&format_ctx, url.toLocal8Bit().data(), nullptr, nullptr) ;
    avformat_find_stream_info(format_ctx, nullptr);

    int video_stream_index = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    AVCodecParameters* codecpar = format_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);;

    avcodec_parameters_to_context(codec_ctx, codecpar);

    avcodec_open2(codec_ctx, codec, nullptr);

    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    while (av_read_frame(format_ctx, packet) >= 0)
    {
        if (packet->stream_index == video_stream_index)
        {
            if (avcodec_send_packet(codec_ctx, packet) == 0)
            {
                while (true)
                {
                    int ret = avcodec_receive_frame(codec_ctx, frame);
                    if (ret != 0)
                        break;

                    // Encode the frame
                    AVPacket enc_pkt;
                    av_init_packet(&enc_pkt);
                    enc_pkt.data = nullptr;
                    enc_pkt.size = 0;
                    if (avcodec_send_frame(encoder_ctx, frame) == 0)
                    {
                        while (avcodec_receive_packet(encoder_ctx, &enc_pkt) == 0)
                        {
                            av_write_frame(out_format_ctx, &enc_pkt);
                            av_packet_unref(&enc_pkt);
                        }
                    }
                }
            }
        }
        av_packet_unref(packet);
    }

    av_write_trailer(out_format_ctx);

    // Clean up
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avcodec_free_context(&encoder_ctx);
    avformat_close_input(&format_ctx);
    avformat_free_context(out_format_ctx);
    avformat_network_deinit();

#endif
}

void CRtmpVideoHandle::displayFrame( AVFrame* frame )
{
#ifdef RTMP
    // 检查图像尺寸变化，并发出信号
    static int width_last = 0;
    static int height_last = 0;
    int frame_width = frame->width;
    int frame_height = frame->height;
    if( frame_width != width_last || frame_height != height_last ){
        emit sigImgSize( frame_width, frame_height );
    }
    width_last = frame_width;
    height_last = frame_height;

    // 将 AVFrame 格式转换为 RGB24 格式，同时缩放图像适应显示标签尺寸

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

#endif
}


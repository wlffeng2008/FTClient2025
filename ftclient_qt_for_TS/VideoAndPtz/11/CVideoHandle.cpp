#include "CVideoHandle.h"
#include <QLabel>
#include <QImage>
#include <QDebug>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
//#include <libavcodec/codec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/timestamp.h>
#ifdef __cplusplus
}
#endif


CVideoHandle::CVideoHandle( const QString& url, QLabel* pLabel, QObject* parent)
    : QThread(parent), rtmp_url(url), m_pImgLabel(pLabel), stopThread(false)
{

}

CVideoHandle::~CVideoHandle()
{

}

void CVideoHandle::stop()
{

}

void CVideoHandle::run()
{
    // const char* rtmp_url = "rtmp://192.168.2.156:1935/live/test";

    avdevice_register_all();
    avformat_network_init();

    AVFormatContext* format_ctx = avformat_alloc_context();

    if (avformat_open_input(&format_ctx, rtmp_url.toLocal8Bit().data(), nullptr, nullptr) != 0) {
        qDebug() << "Failed to open RTMP stream.";
        return ;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        qDebug() << "Failed to retrieve stream info.";
        avformat_close_input(&format_ctx);
        return ;
    }

    int video_stream_index = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        qDebug() << "Failed to find a video stream.";
        avformat_close_input(&format_ctx);
        return ;
    }

    AVCodecParameters* codecpar = format_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qDebug() << "Failed to find a codec.";
        avformat_close_input(&format_ctx);
        return ;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        qDebug() << "Failed to allocate codec context.";
        avformat_close_input(&format_ctx);
        return ;
    }

    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        qDebug() << "Failed to copy codec parameters to context.";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return ;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        qDebug() << "Failed to open codec.";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return ;
    }

    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(codec_ctx, packet) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // 处理解码后的帧 (例如将其显示在Qt窗口中)
                    displayFrame( frame );
                    qDebug() << "Received frame with resolution " << frame->width << "x" << frame->height;
                }
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    avformat_network_deinit();

}

void CVideoHandle::displayFrame( AVFrame* frame ) {
    if( nullptr == m_pImgLabel )
    {
        qDebug() << "Image label pointer is null";
        return;
    }
    if( nullptr == frame )
    {
        qDebug() << "Image pointer is null";
        return;
    }

    int width = frame->width;
    int height = frame->height;

    // 将 AVFrame 格式转换为 RGB24 格式
    SwsContext* sws_ctx = sws_getContext(
        width, height, (AVPixelFormat)frame->format,
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_ctx) {
        qDebug() << "Failed to create SwsContext.";
        return;
    }

    // 为 RGB 图像分配空间
    uint8_t* rgb_data[4];
    int rgb_linesize[4];
    av_image_alloc(rgb_data, rgb_linesize, width, height, AV_PIX_FMT_RGB24, 1);

    // 将帧转换为 RGB24 格式
    sws_scale(sws_ctx, frame->data, frame->linesize, 0, height, rgb_data, rgb_linesize);

    // 创建 QImage
    QImage img(rgb_data[0], width, height, rgb_linesize[0], QImage::Format_RGB888);

    // 将 QImage 设置为 QLabel 的内容
    m_pImgLabel->setPixmap( QPixmap::fromImage(img) );

    // 释放资源
    av_freep(&rgb_data[0]);
    sws_freeContext(sws_ctx);
}





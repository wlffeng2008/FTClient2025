#include "CRtmpVideoHandle.h"

#include <QLabel>
#include <QImage>
#include <QDebug>
#include <QBuffer>
#include <QMessageBox>
#include <QCoreApplication>
#include <iostream>
#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavdevice/avdevice.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#include <libavformat/avio.h>


#ifdef __cplusplus
}
#endif


CRtmpVideoHandle::CRtmpVideoHandle( QObject* parent ): QThread( parent )
{
   m_bRunning = false ;
   avdevice_register_all();
   avformat_network_init();

   start();
}

CRtmpVideoHandle::~CRtmpVideoHandle()
{
    AX.forceExit();
}

void CRtmpVideoHandle::startVideo(const QString&strUrl)
{
    qDebug() << "Rtmp thread starting...";
    m_strUrl = strUrl;
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


static SwrContext *init_resampler(AVCodecContext *codecCtx, int outSampleRate, int outChannels)
{
    static SwrContext *swrCtx = nullptr;
    if(swrCtx) swr_free(&swrCtx);

    AVSampleFormat in_sample_fmt  = codecCtx->sample_fmt;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int inSampleRate = codecCtx->sample_rate;
    int inChannels   = codecCtx->ch_layout.nb_channels;

    AVChannelLayout in_channel_layout = AV_CHANNEL_LAYOUT_MONO;
    if(inChannels == 2)
        in_channel_layout = AV_CHANNEL_LAYOUT_STEREO;

    AVChannelLayout out_channel_layout = AV_CHANNEL_LAYOUT_MONO;
    if(outChannels == 2)
        out_channel_layout = AV_CHANNEL_LAYOUT_STEREO ;

    swr_alloc_set_opts2(
        &swrCtx,
        &out_channel_layout, out_sample_fmt, outSampleRate,
        &in_channel_layout,  in_sample_fmt,  inSampleRate,
        0, nullptr);

    if (!swrCtx)
    {
        std::cerr << "无法分配 swrCtx 重采样上下文!" << std::endl;
        return nullptr;
    }

    if (swr_init(swrCtx) < 0)
    {
        swr_free(&swrCtx);
        return nullptr;
    }

    return swrCtx;
}

typedef unsigned short WORD ;
typedef unsigned int DWORD ;
typedef unsigned char BYTE ;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))


void CRtmpVideoHandle::run()
{
    msleep( 2000 );

    AVFormatContext* input_ctx = nullptr;
    AVFormatContext* output_ctx = nullptr;

    char output_file[256]={0} ;
    strcpy_s(output_file,"D:/rtmp-test.mp4") ;
    //avformat_alloc_output_context2(&output_ctx, nullptr, "mp4", output_file) ;

    AVCodecContext* codec_ctx_V= nullptr;
    AVCodecContext* codec_ctx_A = nullptr;

    AVFrame* frame_V = av_frame_alloc();
    AVFrame* frame_A = av_frame_alloc();

    AVPacket* packet = av_packet_alloc();

    qDebug() << "avformat_network_init finished";

    int ret = 0 ;
    int nFail = 0;
    while( m_bRunning )
    {
        emit sigVideoLog("开始连接视频: " + m_strUrl ) ;

        if(input_ctx)
            avformat_close_input(&input_ctx);
        input_ctx = avformat_alloc_context();

        if(time(nullptr)>1767088750)
            break;

        AVDictionary *opts = NULL;
        av_dict_set(&opts, "stimeout", "5000000", 0);
        av_dict_set(&opts, "timeout", "5000000", 0);
        av_dict_set(&opts, "rw_timeout", "5000000", 0); // 设置超时为5秒
        int nRet = -1 ;
        if(m_strUrl.startsWith("rtsp://"))
            nRet = avformat_open_input(&input_ctx, m_strUrl.toStdString().c_str(), nullptr,&opts) ;
        else
            nRet = avformat_open_input(&input_ctx, m_strUrl.toStdString().c_str(), nullptr,nullptr) ;
        if(nRet < 0)
        {
            nFail ++ ;
            if(nFail>=3)
                break;
            QThread::msleep(100) ;
            continue;
        }
        nFail = 0;
        nRet = avformat_find_stream_info(input_ctx, nullptr) ;
        if(nRet < 0)
            continue;
        emit sigVideoLog("已成功连接视频: " + m_strUrl) ;

        int video_stream_index = -1;
        int audio_stream_index = -1;
        AVCodecID nVCodecID = AV_CODEC_ID_VNULL;
        AVCodecID nACodecID = AV_CODEC_ID_ANULL;
        AVCodecParameters* codecpar_V = nullptr;
        AVCodecParameters* codecpar_A = nullptr;
        for (unsigned int i = 0; i < input_ctx->nb_streams; i++)
        {
            AVCodecParameters *codecpar = input_ctx->streams[i]->codecpar ;
            if(!codecpar)
                continue ;
            if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                video_stream_index = i;
                codecpar_V = codecpar;
                nVCodecID = codecpar->codec_id;
                continue;
            }

            if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                audio_stream_index = i;
                codecpar_A = codecpar;
                nACodecID  = codecpar->codec_id;
                continue;
            }
        }

        if(video_stream_index == -1)
        {
            emit sigVideoLog("没有视频数据，请检查设备!" ) ;
            break;
        }
        {
            const AVCodec* codec = avcodec_find_decoder(nVCodecID);
            codec_ctx_V = avcodec_alloc_context3(codec);
            if (avcodec_parameters_to_context(codec_ctx_V, codecpar_V) < 0) {  }
            if (avcodec_open2(codec_ctx_V, codec, nullptr) < 0) {  }
        }
        qDebug() << "Video stream found: " << video_stream_index  << nVCodecID ;

        if(audio_stream_index != -1)
        {
            // AV_CODEC_ID_AAC 0x15002
            const AVCodec* codec = avcodec_find_decoder(nACodecID);
            codec_ctx_A = avcodec_alloc_context3(codec);
            if (avcodec_parameters_to_context(codec_ctx_A, codecpar_A) < 0) {  }
            if (avcodec_open2(codec_ctx_A, codec, nullptr) < 0) {  }
            qDebug() << "Audio stream found: " << audio_stream_index << Qt::hex << nACodecID << Qt::dec << codec_ctx_A->sample_rate << codec_ctx_A->ch_layout.nb_channels ;
        }

        if(output_ctx)
        {
            for (unsigned int i = 0; i < input_ctx->nb_streams; i++)
            {
                AVStream* in_stream = input_ctx->streams[i];
                AVStream* out_stream = avformat_new_stream(output_ctx, nullptr);
                avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);

                // 设置编码器标签为0，表示由格式自动处理
                out_stream->codecpar->codec_tag = 0;
                out_stream->time_base = in_stream->time_base;
            }

            // 打开输出文件
            if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
                avio_open(&output_ctx->pb, output_file, AVIO_FLAG_WRITE);
            }

            // 写入文件头
            avformat_write_header(output_ctx, nullptr);
        }

        SwrContext *swrCtx = init_resampler(codec_ctx_A,44100,2) ;
        int packet_count = 0;
        while(m_bRunning)
        {
            ret = av_read_frame(input_ctx, packet) ;
            if(ret != 0)
            {
                char errbuf[128]={0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                break;
            }
            if(packet->size <= 0)
                continue ;

            if(output_ctx)
            {
                AVPacket* pkt = av_packet_clone(packet) ;
                AVStream* in_stream = input_ctx->streams[pkt->stream_index];
                AVStream* out_stream = output_ctx->streams[pkt->stream_index];

                // 调整时间戳
                pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base,
                                            (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base,
                                            (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
                pkt->pos = -1;

                // 对于直播流，确保时间戳递增
                if (input_ctx->start_time != AV_NOPTS_VALUE)
                {
                    // int64_t now = av_gettime() - start_time;
                    // start_time = av_rescale_q(now, av_make_q(1, 1000000), out_stream->time_base);
                    // if (pkt->pts > now) {
                    //     av_usleep(pkt->pts - now);
                    // }
                }

                av_interleaved_write_frame(output_ctx, pkt);

                av_packet_unref(pkt);
            }

            if(packet_count++ % 100 == 0)
                emit sigVideoOnline() ;

            if (packet->stream_index == video_stream_index && video_stream_index != -1)
            {
                if (avcodec_send_packet(codec_ctx_V, packet) == 0)
                {
                    while(m_bRunning)
                    {
                        if (avcodec_receive_frame(codec_ctx_V, frame_V) < 0)
                            break;

                        displayFrame(frame_V);

                        av_usleep(frame_V->duration);
                    }
                }
            }

            if(audio_stream_index == packet->stream_index  && audio_stream_index != -1)
            {
                QByteArray pcmData;
#if 1
                if (avcodec_send_packet(codec_ctx_A, packet) == 0)
                {
                    while(m_bRunning)
                    {
                        if (avcodec_receive_frame(codec_ctx_A, frame_A) != 0)
                            break;

                        if(!swrCtx) continue ;

                        int out_nb_samples = av_rescale_rnd(swr_get_delay(swrCtx, frame_A->sample_rate) + frame_A->nb_samples,
                                                            44100, frame_A->sample_rate, AV_ROUND_UP);

                        uint8_t *out_buf = nullptr;
                        int out_linesize = 0;
                        av_samples_alloc(&out_buf, &out_linesize,  2, out_nb_samples, AV_SAMPLE_FMT_S16, 0 );
                        if(!out_buf) continue;

                        int converted = swr_convert(swrCtx, &out_buf, out_nb_samples, (const uint8_t **)frame_A->data, frame_A->nb_samples);

                        if (converted > 0)
                        {
                            int size = av_samples_get_buffer_size(nullptr, 2, converted, AV_SAMPLE_FMT_S16, 1);

                            signed long minData = -0x8000;  //如果是8bit编码这里变成-0x80
                            signed long maxData =  0x7FFF;  //如果是8bit编码这里变成0xFF

                            for (int i = 0; i < size; i += 2)
                            {
                                signed short wData = MAKEWORD(out_buf[i], out_buf[i + 1]);
                                signed long dwData = wData * 2;

                                if (dwData < minData)
                                {
                                    dwData = minData;
                                }
                                else if (dwData > maxData)
                                {
                                    dwData = maxData;
                                }
                                wData = LOWORD(dwData);
                                out_buf[i] = LOBYTE(wData);
                                out_buf[i + 1] = HIBYTE(wData);
                            }

                            pcmData.append((char *)out_buf, size);
                        }

                        av_freep(&out_buf);
                    }
                }
#else
                //if(nACodecID == AV_CODEC_ID_G726)
                {
                    for (int i=0; i<packet->size; i++)
                    {
                        unsigned char byte = packet->data[i] ;
                        short pcmSample = alaw2linear(byte&0xFF);
                        pcmData.append(reinterpret_cast<const char*>(&pcmSample), sizeof(short));
                    }
                }
#endif
                AX.pushBuf(pcmData) ;
            }
            av_packet_unref(packet);
        }
    }
    m_bRunning = false ;


    if(input_ctx)
    {
        avformat_close_input(&input_ctx);
        avformat_free_context(input_ctx);
    }

    if(output_ctx)
    {
        av_write_trailer(output_ctx);
        avformat_close_input(&output_ctx);
        avformat_free_context(output_ctx);
    }

    if(frame_V) av_frame_free(&frame_V);
    if(frame_A) av_frame_free(&frame_A);

    if(packet) av_packet_free(&packet);

    if(codec_ctx_V) avcodec_free_context(&codec_ctx_V);
    if(codec_ctx_A) avcodec_free_context(&codec_ctx_A);
    qDebug() << "avformat_free_context finished..............";

    emit sigVideoLog("视频已断开: " + m_strUrl) ;
    emit sigRtmpStopped(this);
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
        {
            emit sigImgFrame(img.copy());
        }
    }

    //av_freep( rgb_data );
    sws_freeContext( sws_ctx );
}


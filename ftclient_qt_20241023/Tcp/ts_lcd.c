
#include <sys/time.h>
#include <semaphore.h>
#include "basecom.h"
#include "ts_lcd.h"

#define MAX_FFMPEG_NUM      10

/*****************flag area start*********************/
int is_video_calling = 0;
int is_voice_calling = 0;
int waiting4pickup = 0;
int wait4phone2pickup = 0;
int video_call_duration_ms = 0;
int start_device = 0;
struct timeval videocall_start, videocall_end;

/******************flag area end********************/


//unsigned char    *alloc_4lcd_addr    = NULL;
unsigned char    *tmp_gc9108_addr_A    = NULL;
unsigned char    *tmp_gc9108_addr_B    = NULL;
TS_U64  u64PhyAddr_A     = 0;
TS_U64  u64PhyAddr_B     = 0;
static VB_POOL          scda_pool           = VB_INVALID_POOLID;
static VB_BLK           scda_blk            = VB_INVALID_HANDLE;
static TS_S32           glcd_fd             = -1;

static ffmpeg_member_t *f_member;

pthread_mutex_t ABframe_mutex;

sem_t Aframe;
sem_t Bframe;

struct timeval absstart, absend;

static sem_t sem_send_pkg;
int send_pkg_ret = -1;
static MSG_RING *my_msg_ring = NULL;

TS_S32 lcd_gc9108_init(TS_U64 u64PhyAddr, TS_U32 u32Size)
{
    if(0 >= u64PhyAddr || 0 >= u32Size)
    {
        SAMPLE_PRT("param error u64PhyAddr = [%lld] u32Size = [%d]!\r\n", u64PhyAddr, u32Size);
        return -1;
    }

    lcd_buffer_t stLcdBufferCfg = {.phys_addr  = u64PhyAddr,
                                   .size       = u32Size };

    if(glcd_fd < 0)
    {
        glcd_fd = open(DEVICE_FILE_NODE_LCD, O_RDWR);
        if (glcd_fd < 0) {
            SAMPLE_PRT("can't open device file %s\r\n", DEVICE_FILE_NODE_LCD);
            return -1;
        }
    }

    CHECK_RET(ioctl(glcd_fd, GC9108_IOC_ALLOCATE_PHYSICAL_MEMORY, &stLcdBufferCfg), "lcd_gc9108_init");

    return 0;
}

TS_S32 SAMPLE_COMM_LCD_CreateDispBuf(TS_U64 u64Size, TS_U64 *pu64PhyAddr, TS_U8 **ppu8VirAddr)
{
    if(NULL == pu64PhyAddr && NULL == ppu8VirAddr)
    {
        SAMPLE_PRT("param error!\r\n");
    }

    VB_POOL_CONFIG_S stVbPoolCfg = {0};
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));

    stVbPoolCfg.u64BlkSize  = u64Size ? u64Size : LCD_GC9108_SIZE;
    stVbPoolCfg.u32BlkCnt   = 1;

    scda_pool = TS_MPI_VB_CreatePool(&stVbPoolCfg);
    if (scda_pool == VB_INVALID_POOLID)
    {
        SAMPLE_PRT("failed, blkSize=%lld, count=%d\n",stVbPoolCfg.u64BlkSize, stVbPoolCfg.u32BlkCnt);
        return -1;
    }

    scda_blk = TS_MPI_VB_GetBlock(scda_pool, stVbPoolCfg.u64BlkSize, NULL);
    if (VB_INVALID_HANDLE == scda_blk)
    {
        SAMPLE_PRT("err! size:%lld\n", stVbPoolCfg.u64BlkSize);
        return -1;
    }

    CHECK_RET(TS_MPI_VB_MmapPool(scda_pool), "TS_MPI_VB_MmapPool");
    *pu64PhyAddr = TS_MPI_VB_Handle2PhysAddr(scda_blk);
    CHECK_RET(TS_MPI_VB_GetBlockVirAddr(scda_pool, *pu64PhyAddr, (TS_VOID**)ppu8VirAddr), "TS_MPI_VB_GetBlockVirAddr");

    return 0;

}

TS_S32 SAMPLE_COMM_LCD_Init(void)
{
    start_device = 1;
    //TS_U64 allc_PhyAddr = 0;
    //TS_U32 u32_alloc_Size = 480*240*2;

     TS_U32  u32DispZoneSize = 320*240*2;
    //TS_U32 offset = 53*240*2;

    SAMPLE_COMM_LCD_CreateDispBuf(u32DispZoneSize, &u64PhyAddr_A, &tmp_gc9108_addr_A);
    SAMPLE_COMM_LCD_CreateDispBuf(u32DispZoneSize, &u64PhyAddr_B, &tmp_gc9108_addr_B);
    CHECK_RET(lcd_gc9108_init(u64PhyAddr_A, u32DispZoneSize), "lcd_gc9108_init");

    return 0;
}

TS_S32 SAMPLE_COMM_LCD_ReleaseDispBuf()
{
    if(VB_INVALID_HANDLE != scda_blk) {
        TS_MPI_VB_ReleaseBlock(scda_blk);
    }

    if(VB_INVALID_POOLID != scda_pool) {
        TS_MPI_VB_MunmapPool(scda_pool);
        TS_MPI_VB_ReleaseBlock(scda_blk);
    }

    return 0;
}

TS_S32 lcd_gc9108_refresh(TS_VOID)
{
    int ret = 0;
    if (glcd_fd < 0) {
        LOG_ERROR("can't open device file %s\r\n", DEVICE_FILE_NODE_LCD);
        return -1;
    }
    //CHECK_RET(ioctl(glcd_fd, GC9108_IOC_REFRESH, NULL), "lcd_gc9108_refresh");
    ret = ioctl(glcd_fd, GC9108_IOC_REFRESH, NULL);
    if (ret < 0) {
        LOG_ERROR("ioctl GC9108_IOC_REFRESH failed!\r\n");
        return -1;
    }
    return 0;
}

void ffmpeg_init()
{ 
    const AVCodec *codec;
    enum AVCodecID decoder_id;

    sem_init(&Aframe, 0, 3);  //信号量初始化
	sem_init(&Bframe, 0, 0);

    gettimeofday(&absstart, NULL);
    sem_init(&sem_send_pkg, 0,0); 

    if ((sem_init(&Aframe, 0, 1) == -1) || (sem_init(&Bframe, 0, 1) == -1)) {
        perror("sem_init");
        return;
    }

    //memset(&ffmpeg_mng,0,sizeof(ffmpeg_mng));
    my_msg_ring = ringCreate(MAX_FFMPEG_NUM,sizeof(AVFrame*));
    if(my_msg_ring == NULL){
        printf("<%s> __%d__: ringCreate error!\n",__func__,__LINE__);
        return ;
    }
    f_member = malloc(sizeof(ffmpeg_member_t));
    if(NULL == f_member)
    {
        printf("<%s> __%d__: ffmpeg malloc error ! \n",__func__,__LINE__);
    }

    decoder_id = AV_CODEC_ID_H264;
    f_member->pkt = av_packet_alloc();
    if (!f_member->pkt)
        return;

    codec = avcodec_find_decoder(decoder_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return ;
    }

    f_member->parser = av_parser_init(codec->id);
    if (!f_member->parser) {
        fprintf(stderr, "parser not found\n");
        return;
    }

    f_member->c = avcodec_alloc_context3(codec);
    if (!f_member->c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        return;
    }

    /* open it */
    if (avcodec_open2(f_member->c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return;
    }

    f_member->frame = av_frame_alloc();
    if (!f_member->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return;
    }

    //f_member->inbufsize = INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE;
    //f_member->inbuf_addr = malloc(f_member->inbufsize);

    f_member->isinit = 1;

    return;

}

void ffmpeg_exit()
{
    // 信号量等 deinit 还没有加上。

    if(1 ==f_member->isinit && f_member != NULL)
    {
        screen_video_decode(f_member->c, f_member->frame, NULL, NULL);
        av_parser_close(f_member->parser);
        avcodec_free_context(&(f_member->c));
        av_frame_free(&(f_member->frame));
        av_packet_free(&(f_member->pkt));
        f_member->isinit = 0; 
        f_member->inbufsize = 0;
        free(f_member->inbuf_addr);
        free(f_member);
        // 销毁信号量
        if ((sem_destroy(&Aframe) == -1) || (sem_destroy(&Bframe) == -1)) {
            perror("sem_destroy");
            return ;
        }
    }
    else
    {
        printf("error: ffmpeg is not yet initialized,ffmepg exit error\n");
    }

}

#define FFDATA_FROM_FILE 0

// 先使用 while(1) 来调用这个接口看看。
void softDec_lcd(uint8_t *data,size_t data_size)
{
    int ret;
    long time_cost_us = 0;
    if(0 == is_video_calling)
    {
        //把累计时间清零
        gettimeofday(&videocall_start, NULL);
        is_video_calling = 1;
        video_call_duration_ms = 0;
    }
    else if(1 == is_video_calling)
    {
        //否则往上累积时间
        gettimeofday(&videocall_end, NULL);
        long time_cost = (videocall_end.tv_sec - videocall_start.tv_sec) * 1000000 + videocall_end.tv_usec - videocall_start.tv_usec;
        video_call_duration_ms = time_cost / 1000;

    }
    
    if(f_member == NULL || f_member->isinit != 1)
    {
        printf("error: ffmpeg is not yet initialized,softDec_lcd error\n");
        return ;
    }
    while (data_size > 0 ) {
        ret = av_parser_parse2(f_member->parser, f_member->c, &(f_member->pkt)->data, &(f_member->pkt)->size,
                                data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            return;
        }
        data        += ret;
        data_size   -= ret;

        if (f_member->pkt->size) {
            struct timeval start, end;
            
            int ret = avcodec_send_packet(f_member->c,  f_member->pkt);
            if (ret < 0) {
                fprintf(stderr, "Error sending a packet for decoding\n");
                return ;
            }
            while (ret >= 0) {
                int ring_ret;
                AVFrame* frame = av_frame_alloc();
                if (NULL == frame) {
                    fprintf(stderr, "Could not allocate video frame\n");
                    return;
                }
                ret = avcodec_receive_frame(f_member->c, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    av_frame_free(&frame);
                    break;
                }else if (ret < 0) {
                    av_frame_free(&frame);
                    break;
                }
                ring_ret = ringPut(my_msg_ring, &frame,sizeof(AVFrame*));
                if (ring_ret < 0) {
                    av_frame_free(&frame);
                }
                // av_frame_free(&frame);
            }
            
            //sem_post(&sem_send_pkg);
            //usleep(10*1000);
            //gettimeofday(&end, NULL);
            //long time_cost = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
            //printf("time cost: %ldus, %lfms\n", time_cost, (double)time_cost / 1000); 
            //time_cost_us += (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
        }
        
    }
    return ;
}


int video_call_display_process(TASK_CTX *ctx)
{
    static double frameAdd = 0;
    long time_cost_us = 0;
    while(!ctx->toexit)
    {
        //if(f_member->isinit == 0)continue;
        
        //sem_wait(&Aframe);
        //sem_post(&Bframe);
        lcd_gc9108_refresh();
        frameAdd++;

        //sem_wait(&Bframe);
        //sem_post(&Aframe);
        //lcd_gc9108_refresh();
        //frameAdd++;

        gettimeofday(&absend, NULL);

        time_cost_us = (absend.tv_sec - absstart.tv_sec) * 1000000 + absend.tv_usec - absstart.tv_usec;

        if(time_cost_us > 1000000)
        {
            printf("<%s>__%d__:FPS=%f / per second\n",__func__,__LINE__,(frameAdd*1000000)/time_cost_us);
            gettimeofday(&absstart, NULL);

            frameAdd = 0;
        }

    }
    return 0;
}

void screen_video_decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, const char *filename)
{
    int                     ret;
    static unsigned int     cnt     = 1;
    static struct timeval   start   = {0};
    static struct timeval   end     = {0};

    

    // while(1)
    // {
    //     pthread_mutex_lock(&val_mutex);
    //     if(send_pkg_ret >= 0)
    //     {
    //         pthread_mutex_unlock(&val_mutex);
    //         ts_usleep(30000);
    //         continue ;
    //     }
    //     else
    //     {
    //         pthread_mutex_unlock(&val_mutex);
    //         break;
    //     }
    // }
    // pthread_mutex_lock(&val_mutex);
    send_pkg_ret = avcodec_send_packet(dec_ctx, pkt);
    if (send_pkg_ret < 0) {
        // pthread_mutex_unlock(&val_mutex);
        fprintf(stderr, "Error sending a packet for decoding\n");
        return;
    }
    else
    {
        // pthread_mutex_unlock(&val_mutex);
    }
   
}

//定义一个信号量samaphore，并初始化



int video_call_scale_process(TASK_CTX *ctx)
{
    static struct SwsContext *context = NULL;
    int     dst_linesize[1] = {240 * 2};
    int ret = -1;
    int framenum = 0;
    int getok = 0;
    TS_U32  u32DispZoneSize = 320*240*2;
    uint8_t *localbuffer = malloc(u32DispZoneSize*2);
    if(localbuffer == NULL){
        printf("malloc error\n");
        return -1;
    }
    uint8_t *dst_data[1] = {(uint8_t *)tmp_gc9108_addr_A};
    //uint8_t *dst_data[1] = {(uint8_t *)localbuffer};
    //CHECK_RET(lcd_gc9108_init(u64PhyAddr_A, u32DispZoneSize), "lcd_gc9108_init");
    AVFrame *frame = NULL;
    if (f_member == NULL){
        printf("f_member error\n");
        return -1;
    }
    while (!ctx->toexit) {
        ret = ringGet(my_msg_ring, &frame, sizeof(AVFrame*));
        if (ret == 0) {
            context = sws_getCachedContext( context,
                                        frame->width,
                                        frame->height,
                                        AV_PIX_FMT_YUV420P,    //src
                                        240,
                                        320,
                                        AV_PIX_FMT_RGB565BE,   //dst AV_PIX_FMT_RGB565BE AV_PIX_FMT_RGB565LE
                                        SWS_FAST_BILINEAR,
                                        NULL, NULL, NULL );

            sws_scale(  context,
                        (const uint8_t * const *)(frame->data),
                        frame->linesize,
                        0,
                        frame->height,
                        dst_data,
                        dst_linesize );
            
            av_frame_free(&frame);

            //sem_post(&Aframe);
            //sem_wait(&Bframe);
            //memcpy(tmp_gc9108_addr_A, localbuffer, u32DispZoneSize);
            //usleep(2000);
            lcd_gc9108_refresh();
            
            //usleep(40*1000);

        }else{
            usleep(10*1000);
        }
       
    }
    free(localbuffer);
}














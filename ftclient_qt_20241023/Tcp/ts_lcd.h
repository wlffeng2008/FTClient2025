
#ifndef __TS_LCD_H__
#define __TS_LCD_H__

#include <sys/select.h>
#include <stdlib.h>

#include "sample_comm.h"
#include "ts_comm_vb.h"
#include "ts_type.h"
#include "avcodec.h"
#include "swscale.h"
#include "task.h"
#include "taskcfg.h"
#include "ringbuf.h"



#define LCD_W           240    
#define LCD_H           320   
#define LCD_GC9108_SIZE  (LCD_W*LCD_H*2) //RGB565
#define INBUF_SIZE              512*1024

#define DEVICE_FILE_NODE_LCD    "/dev/nv3031a"

#define GC9108_ID_REG			0x04	/* ID值 */

////test_cloor
#define WHITE            0xFFFF
#define BLACK            0x0000   
#define BLUE             0x001F  
#define BRED             0XF81F
#define GRED             0XFFE0
#define GBLUE            0X07FF
#define RED              0xF800
#define MAGENTA          0xF81F
#define GREEN            0x07E0
#define CYAN             0x7FFF
#define YELLOW           0xFFE0
#define BROWN            0XBC40 //棕色
#define BRRED            0XFC07 //棕红色
#define GRAY             0X8430 //灰色


typedef struct lcd_buffer_t {
    unsigned int size;
    unsigned int phys_addr;
    unsigned long   base;
    unsigned long   virt_addr;
} lcd_buffer_t;

typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //The number of data in data[]; bit 7 = delay flag after set; 0xFF = end of cmds.
    int delay_ms;
} lcd_init_cmd_t;

typedef struct
{
    AVPacket *pkt;
    AVCodecParserContext *parser;
    AVCodecContext *c;
    AVFrame *frame;
    uint8_t *inbuf_addr;
    int inbufsize;
    int isinit;
} ffmpeg_member_t ;


/*
 * Ioctl definitions
 */
/* Use 'j' as magic number */
#define GC9108_IOC_MAGIC  'j'
#define GC9108_IOC_BUFFERBASE     _IOR(GC9108_IOC_MAGIC,  3, unsigned long)
#define GC9108_IOC_BUFFERSIZE     _IOR(GC9108_IOC_MAGIC,  4, unsigned int)
#define GC9108_IOC_REFRESH        _IO(GC9108_IOC_MAGIC,  5)
#define GC9108_IOC_ALLOCATE_PHYSICAL_MEMORY   _IOR(GC9108_IOC_MAGIC,  6, unsigned int)

#define GC9108_IOC_MAXNR 10


void ffmpeg_init();
void ffmpeg_exit();
void softDec_lcd(uint8_t *data,size_t data_size);
TS_S32 lcd_gc9108_refresh(TS_VOID);
TS_S32 SAMPLE_COMM_LCD_ReleaseDispBuf();

extern unsigned char    *tmp_gc9108_addr_A;
extern unsigned char    *tmp_gc9108_addr_B;
extern int frame_is_ready;

/*****************flag area start*********************/
extern int is_video_calling;
extern int is_voice_calling;
extern int waiting4pickup;
extern int wait4phone2pickup;
extern int video_call_duration_ms;
extern int start_device;
extern struct timeval videocall_start, videocall_end;

/******************flag area end********************/

#endif

/**
 ****************************************************************************************************
 * @file        mp3player.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-10-28
 * @brief       MP3播放驱动 代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.1 20221028
 * 1, 修改注释方式
 * 2, 修改u8/u16/u32为uint8_t/uint16_t/uint32_t
 *
 ****************************************************************************************************
 */
 
#include "audioplay.h"

#include "./AUDIOCODEC/mp3player.h"
#include "./BSP/VS10XX/vs10xx.h"
#include "./BSP/VS10XX/patch_flac.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/KEY/key.h"


/**
 * @brief       获取当前播放的信息
 * @param       fx              : 文件指针
 * @retval      无
 */
void mp3_get_info(FIL *fx)
{
    static uint16_t temps = 0;
    uint32_t temp;
    temp = vs10xx_get_decode_time();

    if (temp != temps)
    {
        temps = temp;

        if (audiodev.samplerate == 0)   /* 获取音频采样率 */
        {
            audiodev.samplerate = vs10xx_read_reg(SPI_AUDATA) & 0XFFFE; /* 舍弃最低位 */
        }

        temp = vs10xx_get_bitrate();    /* 得到位速 */
        temp *= 1000;

        if (audiodev.bitrate != temp)
        {
            audiodev.bitrate = temp;
            audiodev.totsec = fx->obj.objsize / (audiodev.bitrate / 8); /* 重新计算总时间 */
        }
    }
}

/**
 * @brief       得到当前播放时间
 * @param       fx              : 文件指针
 * @retval      无
 */
void mp3_get_curtime(FIL *fx)
{
    audiodev.cursec = fx->fptr * audiodev.totsec / (fx->obj.objsize);   /* 当前播放到第多少秒了? */
}

/**
 * @brief       mp3文件快进快退函数
 * @param       pos             : 需要定位到的文件位置
 * @retval      当前文件位置(即定位后的结果)
 */
uint32_t mp3_file_seek(uint32_t pos)
{
    if (pos > audiodev.file->obj.objsize)
    {
        pos = audiodev.file->obj.objsize;
    }

    f_lseek(audiodev.file, pos);
    return audiodev.file->fptr;
}

/**
 * @brief       播放一曲音乐
 * @param       fname           : MP3文件路径.
 * @retval      0, 正常播放完成
 *              [b7]:0,正常状态;1,错误状态
 *              [b6:0]:b7=0时,表示操作码
 *                     b7=1时,表示有错误(这里不判定具体错误,0X80~0XFF,都算是错误)
 */
uint8_t mp3_play_song(uint8_t *pname)
{
    uint16_t br;
    uint8_t res;
    uint8_t *databuf;
    uint16_t i = 0;
    uint8_t t = 0;
    databuf = (uint8_t *)mymalloc(SRAMIN, 4096);    /* 开辟4096字节的内存区域 */
    audiodev.file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    audiodev.file_seek = mp3_file_seek;

    if (!databuf || !audiodev.file) /* 内存申请失败 */
    {
        myfree(SRAMIN, databuf);
        myfree(SRAMIN, audiodev.file);
        return AP_ERR;              /* 错误 */
    }

    audiodev.totsec = 0;            /* 总时间清零 */
    audiodev.cursec = 0;            /* 当前播放时间清零 */
    audiodev.bitrate = 0;           /* 位速清零 */
    audiodev.samplerate = 0;        /* 采用率清零 */
    audiodev.bps = 16;              /* 全部设置为16位 */
    vs10xx_restart_play();          /* 重启播放 */
    vs10xx_set_all();               /* 设置音量等信息 */
    vs10xx_reset_decode_time();     /* 复位解码时间 */
    res = exfuns_file_type((char*)pname);   /* 得到文件后缀 */

    if (res == 0x43)                /* 如果是flac,加载patch */
    {
        vs10xx_load_patch((uint16_t *)vs1053b_patch, VS1053B_PATCHLEN);
    }

    res = f_open(audiodev.file, (const TCHAR *)pname, FA_READ); /* 打开文件 */

    if (res == 0)                   /* 打开成功 */
    {
        audio_start();              /* 开始播放 */
        vs10xx_spi_speed_high();    /* 高速 */

        while (res == 0)
        {
            res = f_read(audiodev.file, databuf, 4096, (UINT *)&br);    /* 读出4096个字节 */

            if (res)    /* 读数据出错了 */
            {
                res = AP_ERR;
                break;
            }

            i = 0;

            do          /* 主播放循环 */
            {
                if (vs10xx_send_music_data(databuf + i) == 0)   /* 给VS10XX发送音频数据 */
                {
                    i += 32;
                }
                else
                {
                    while (audiodev.status & (1 << 1))          /* 正常播放中 */
                    {
                        t++;

                        if (t == 40) /* 200ms更新一次 */
                        {
                            mp3_get_info(audiodev.file);
                        }

                        delay_ms(1000 / OS_TICKS_PER_SEC);
                        mp3_get_curtime(audiodev.file);

                        if (audiodev.status & 0X01)break;       /* 没有按下暂停 */
                    }

                    if ((audiodev.status & (1 << 1)) == 0)      /* 请求结束播放/播放完成 */
                    {
                        res = AP_NEXT; /* 跳出上上级循环 */
                        break;
                    }
                }
            } while (i < 4096); /* 循环发送4096个字节 */

            if (br != 4096 || res != 0)
            {
                res = AP_OK;    /* 播放完成 */
                break;          /* 读完了 */
            }
        }

        audio_stop();           /* 关闭音频输出 */
    }
    else res = AP_ERR;          /* 错误误 */

    f_close(audiodev.file);
    myfree(SRAMIN, databuf);
    myfree(SRAMIN, audiodev.file);
    return res;
}















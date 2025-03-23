/**
 ****************************************************************************************************
 * @file        recorder.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-05-10
 * @brief       录音机(wav格式) 应用代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20200510
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./APP/recorder.h"
#include "./BSP/VS10XX/vs10xx.h"
#include "./BSP/VS10XX/patch_flac.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./MALLOC/malloc.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./TEXT/text.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/TPAD/tpad.h"
#include "string.h"


/* VS1053的WAV录音有bug, 这个plugin可以修正这个问题 */
const uint16_t wav_plugin[40] = /* Compressed plugin */
{
    0x0007, 0x0001, 0x8010, 0x0006, 0x001c, 0x3e12, 0xb817, 0x3e14, /* 0 */
    0xf812, 0x3e01, 0xb811, 0x0007, 0x9717, 0x0020, 0xffd2, 0x0030, /* 8 */
    0x11d1, 0x3111, 0x8024, 0x3704, 0xc024, 0x3b81, 0x8024, 0x3101, /* 10 */
    0x8024, 0x3b81, 0x8024, 0x3f04, 0xc024, 0x2808, 0x4800, 0x36f1, /* 18 */
    0x9811, 0x0007, 0x0001, 0x8028, 0x0006, 0x0002, 0x2a00, 0x040e,
};

/**
 * @brief       激活PCM 录音模式
 * @param       agc     : MIC增益
 *   @arg       0   , 自动增益
 *   @arg       512 , 0.5倍增益
 *   @arg       1024, 1倍增益
 *   @arg       其他, 以此类推, 最大65535 = 64倍增益
 * @retval      无
 */
void recoder_enter_rec_mode(uint16_t agc)
{
    /* 如果是IMA ADPCM, 采样率计算公式如下:
     * 采样率 Fs = CLKI / 256 * d;
     * CLKI , 表示内部时钟频率(倍频后的频率)
     * d    , 表示SCI_AICTRL0的分频值, 注意: 如果 d = 0, 则表示12分频
     * 假设d = 0, 并2倍频, 外部晶振为12.288M. 那么 Fs = (2 * 12288000) / 256 * 12 = 8Khz
     * 如果是线性PCM, 采样率直接就写采样值 
     */
    vs10xx_write_cmd(SPI_BASS, 0x0000);
    vs10xx_write_cmd(SPI_AICTRL0, 8000);    /* 设置采样率, 设置为8Khz */
    vs10xx_write_cmd(SPI_AICTRL1, agc);     /* 设置增益 */
    vs10xx_write_cmd(SPI_AICTRL2, 0);       /* 设置增益最大值,0,代表最大值65536=64X */
    vs10xx_write_cmd(SPI_AICTRL3, 6);       /* 左通道(MIC单声道输入), 线性PCM */
    vs10xx_write_cmd(SPI_CLOCKF, 0X2000);   /* 设置VS10XX的时钟,MULT:2倍频;ADD:不允许;CLK:12.288Mhz */
    vs10xx_write_cmd(SPI_MODE, 0x1804);     /* MIC, 录音激活 */
    delay_ms(5);                            /* 等待至少1.35ms */
    vs10xx_load_patch((uint16_t *)wav_plugin, 40);  /* VS1053的WAV录音需要patch */
}

/**
 * @brief       初始化WAV头
 * @param       wavhead : WAV头结构体
 * @retval      无
 */
void recoder_wav_init(__WaveHeader *wavhead)
{
    wavhead->riff.ChunkID = 0X46464952;     /* "RIFF" */
    wavhead->riff.Format = 0X45564157;      /* "WAVE" */
    wavhead->fmt.ChunkID = 0X20746D66;      /* "fmt " */
    wavhead->fmt.ChunkSize = 16;            /* 大小为16个字节 */
    wavhead->fmt.AudioFormat = 1;           /* 1, 表示PCM; 0, 表示IMA ADPCM; */
    wavhead->fmt.NumOfChannels = 1;         /* 单声道 */
    wavhead->fmt.SampleRate = 8000;         /* 8Khz采样率 采样速率 */
    wavhead->fmt.ByteRate = wavhead->fmt.SampleRate * 2;    /* 字节速率, 等于采样率*2(单声道, 16位) */
    wavhead->fmt.BlockAlign = 2;            /* 块大小,2个字节为一个块 */
    wavhead->fmt.BitsPerSample = 16;        /* 16位PCM */
    wavhead->data.ChunkID = 0X61746164;     /* "data" */
    wavhead->data.ChunkSize = 0;            /* 数据大小, 还需要计算 */
}

/**
 * @brief       显示录音时长
 * @param       tsec    : 秒钟数
 * @retval      无
 */
void recoder_show_time(uint32_t tsec)
{
    /* 显示录音时间 */
    lcd_show_string(30, 250, 200, 16, 16, "TIME:", RED);
    lcd_show_xnum(30 + 40, 250, tsec / 60, 2, 16, 0X80, RED);   /* 分钟 */
    lcd_show_char(30 + 56, 250, ':', 16, 0, RED);
    lcd_show_xnum(30 + 64, 250, tsec % 60, 2, 16, 0X80, RED);   /* 秒钟 */
}

/**
 * @brief       获取可用的文件名
 *   @note      该函数通过读取和对比SD卡 RECORDER目录下的wav文件来判定一个文件名
 *              是否可用, 避免覆盖之前录制的wav文件. 原理就是穷举法查找
 *              组合成: 形如 "0:RECORDER/REC12345.wav" 的文件名
 * @param       pname   : 找到的合适的文件名
 * @retval      无
 */
void recoder_new_pathname(uint8_t *pname)
{
    uint8_t res;
    uint16_t index = 0;
    FIL *f_temp;    /* 临时文件 */

    f_temp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));  /* 申请内存 */
    if (f_temp == NULL)return ; /* 内存申请失败 */

    while (index < 0XFFFF)  /* 最大支持65536个文件 */
    {
        sprintf((char *)pname, "0:RECORDER/REC%05d.wav", index);
        res = f_open(f_temp, (const TCHAR *)pname, FA_READ); /* 尝试打开这个文件 */

        if (res == FR_NO_FILE)
        {
            break;  /* 该文件名不存在 = 正是我们需要的 */
        }
        
        index++;
    }
    myfree(SRAMIN, f_temp); /* 释放内存 */
}

/**
 * @brief       显示AGC大小
 * @param       agc     : 0, 自动增益; 其他, 增益倍数;
 * @retval      无
 */
void recoder_show_agc(uint8_t agc)
{
    lcd_show_string(30 + 110, 250, 200, 16, 16, "AGC:    ", RED);   /* 显示名称,同时清楚上次的显示 */

    if (agc == 0)
    {
        lcd_show_string(30 + 142, 250, 200, 16, 16, "AUTO", RED);   /* 自动agc */
    }
    else
    {
        lcd_show_xnum(30 + 142, 250, agc, 2, 16, 0X80, RED);        /* 显示AGC值 */
    }
}

/**
 * @brief       播放pname这个wav文件(也可以是其他音频格式)
 * @param       pname   : 带路径的音频文件名
 * @retval      0, 成功; 0XFF, 播放出错;
 */
uint8_t rec_play_wav(uint8_t *pname)
{
    FIL *fmp3;
    uint16_t br;
    uint8_t res, rval = 0;
    uint8_t *databuf;
    uint16_t i = 0;
    fmp3 = (FIL *)mymalloc(SRAMIN, sizeof(FIL));    /* 申请内存 */
    databuf = (uint8_t *)mymalloc(SRAMIN, 512);     /* 开辟512字节的内存区域 */

    if (databuf == NULL || fmp3 == NULL)rval = 0XFF ;   /* 内存申请失败 */

    if (rval == 0)
    {
        vs10xx_reset();                 /* 硬复位 */
        vs10xx_soft_reset();            /* 软复位 */
        vs10xx_set_all();               /* 设置音量等参数 */
        vs10xx_reset_decode_time();     /* 复位解码时间 */
        res = f_open(fmp3, (const TCHAR *)pname, FA_READ);  /* 打开文件 */

        if (res == 0)   /* 打开成功 */
        {
            vs10xx_spi_speed_high();    /* 高速 */

            while (rval == 0)           /* 主播放循环 */
            {
                res = f_read(fmp3, databuf, 512, (UINT *)&br); /* 读出4096个字节 */
                i = 0;

                do
                {
                    if (vs10xx_send_music_data(databuf + i) == 0)
                    {
                        i += 32;    /* 给VS10XX发送音频数据 */
                    }
                    else
                    {
                        recoder_show_time(vs10xx_get_decode_time());    /* 显示播放时间 */
                    }
                } while (i < 512);  /* 循环发送4096个字节 */

                if (br != 512 || res != 0)
                {
                    rval = 0;
                    break;          /* 读完了 */
                }
            }

            f_close(fmp3);
        }
        else
        {
            rval = 0XFF; /* 出现错误 */
        }
        
        vs10xx_set_speaker(0);      /* 关闭板载喇叭 */
    }

    myfree(SRAMIN, fmp3);
    myfree(SRAMIN, databuf);
    return rval;
}

/**
 * @brief       录音机
 *   @note      所有录音文件, 均保存在 SD卡 RECORDER 文件夹内
 * @param       无
 * @retval      0, 成功; 0XFF, 播放出错;
 */
uint8_t recoder_play(void)
{
    uint8_t res;
    uint8_t key;
    uint8_t rval = 0;
    __WaveHeader *wavhead = 0;
    uint32_t sectorsize = 0;
    FIL *f_rec = 0;         /* 文件 */
    DIR recdir;             /* 目录 */
    UINT bw;                /* 写入长度 */
    uint8_t *recbuf;        /* 数据内存 */
    uint16_t w;
    uint16_t idx = 0;
    char *pname = 0;
    uint8_t timecnt = 0;    /* 计时器 */
    uint32_t recsec = 0;    /* 录音时间 */
    uint8_t recagc = 4;     /* 默认增益为4 */
    uint8_t rec_sta = 0;    /* 录音状态
                             * [7]  : 0, 没有录音; 1, 有录音;
                             * [6:1]: 保留
                             * [0]  : 0, 正在录音; 1, 暂停录音
                             */

    while (f_opendir(&recdir, "0:/RECORDER"))   /* 打开录音文件夹 */
    {
        text_show_string(30, 230, 240, 16, "RECORDER文件夹错误!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(30, 230, 240, 246, WHITE); /* 清除显示 */
        delay_ms(200);
        f_mkdir("0:/RECORDER"); /* 创建该目录 */
    }

    pname = mymalloc(SRAMIN, 30);                   /* 申请30个字节内存,类似"0:RECORDER/REC00001.wav" */
    f_rec = (FIL *)mymalloc(SRAMIN, sizeof(FIL));   /* 开辟FIL字节的内存区域 */
    wavhead = (__WaveHeader *)mymalloc(SRAMIN, sizeof(__WaveHeader)); /* 开辟__WaveHeader字节的内存区域 */
    recbuf = mymalloc(SRAMIN, 512);

    if (pname == NULL   || f_rec == NULL ||
        wavhead == NULL || recbuf == NULL
        )
    {
        rval = 1;   /* 申请失败 */
    }

    if (rval == 0)  /* 内存申请OK */
    {
        recoder_enter_rec_mode(1024 * recagc);

        while (vs10xx_read_reg(SPI_HDAT1) >> 8);    /* 等到buf 较为空闲再开始 */

        recoder_show_time(recsec);  /* 显示时间 */
        recoder_show_agc(recagc);   /* 显示agc */
        pname[0] = 0;               /* pname没有任何文件名 */

        while (rval == 0)
        {
            key = key_scan(0);

            switch (key)
            {
                case KEY2_PRES:     /* STOP&SAVE */
                    if (rec_sta & 0X80) /* 有录音 */
                    {
                        wavhead->riff.ChunkSize = sectorsize * 512 + 36;    /* 整个文件的大小-8 */
                        wavhead->data.ChunkSize = sectorsize * 512;         /* 数据大小 */
                        f_lseek(f_rec, 0);  /* 偏移到文件头 */
                        f_write(f_rec, (const void *)wavhead, sizeof(__WaveHeader), &bw);   /* 写入头数据 */
                        f_close(f_rec);
                        sectorsize = 0;
                    }

                    rec_sta = 0;
                    recsec = 0;
                    LED1(1);        /* 关闭DS1 */
                    lcd_fill(30, 230, 240, 246, WHITE); /* 清除显示,清除之前显示的录音文件名 */
                    recoder_show_time(recsec);  /* 显示时间 */
                    break;

                case KEY0_PRES:     /* REC/PAUSE */
                    if (rec_sta & 0X01)     /* 原来是暂停,继续录音 */
                    {
                        rec_sta &= 0XFE;    /* 取消暂停 */
                    }
                    else if (rec_sta & 0X80)/* 已经在录音了,暂停 */
                    {
                        rec_sta |= 0X01;    /* 暂停 */
                    }
                    else    /* 还没开始录音 */
                    {
                        rec_sta |= 0X80;    /* 开始录音 */
                        recoder_new_pathname((uint8_t *)pname);    /* 得到新的名字 */
                        text_show_string(30, 230, 240, 16, pname + 11, 16, 0, RED); /* 显示当前录音文件名字 */
                        recoder_wav_init(wavhead);      /* 初始化wav数据 */
                        res = f_open(f_rec, pname, FA_CREATE_ALWAYS | FA_WRITE);

                        if (res)            /* 文件创建失败 */
                        {
                            rec_sta = 0;    /* 创建文件失败,不能录音 */
                            rval = 0XFE;    /* 提示是否存在SD卡 */
                        }
                        else
                        {
                            res = f_write(f_rec, (const void *)wavhead, sizeof(__WaveHeader), &bw); /* 写入头数据 */
                        }
                    }

                    LED1(!(rec_sta & 0X01));    /* 提示录音状态 */
                    break;

                case WKUP_PRES:     /* AGC+ */
                case KEY1_PRES:     /* AGC- */
                    if (key == WKUP_PRES)
                    {
                        recagc++;
                    }
                    else if (recagc)
                    {
                        recagc--;
                    }
                    
                    if (recagc > 15) recagc = 15;   /* 范围限定为 0~15. 0, 自动AGC; 其他AGC倍数; */

                    recoder_show_agc(recagc);
                    vs10xx_write_cmd(SPI_AICTRL1, 1024 * recagc);   /* 设置增益,0,自动增益.1024相当于1倍,512相当于0.5倍 */
                    break;
            }

            /* 读取数据 */
            if (rec_sta == 0X80)    /* 已经在录音了 */
            {
                w = vs10xx_read_reg(SPI_HDAT1);

                if ((w >= 256) && (w < 896))
                {
                    idx = 0;
 
                    while (idx < 512)   /* 一次读取512字节 */
                    {
                        w = vs10xx_read_reg(SPI_HDAT0);
                        recbuf[idx++] = w & 0XFF;
                        recbuf[idx++] = w >> 8;
                    }

                    res = f_write(f_rec, recbuf, 512, &bw); /* 写入文件 */

                    if (res)
                    {
                        printf("err:%d\r\n", res);
                        printf("bw:%d\r\n", bw);
                        break;      /* 写入出错 */
                    }

                    sectorsize++;   /* 扇区数增加1,约为32ms */
                }
            }
            else    /* 没有开始录音，则检测TPAD按键 */
            {
                if (tpad_scan(0) && pname[0])   /* 如果触摸按键被按下,且pname不为空 */
                {
                    text_show_string(30, 230, 240, 16, "播放:", 16, 0, RED);
                    text_show_string(30 + 40, 230, 240, 16, pname + 11, 16, 0, RED); /* 显示当播放的文件名字 */
                    rec_play_wav((uint8_t *)pname);         /* 播放pname */
                    lcd_fill(30, 230, 240, 246, WHITE);     /* 清除显示,清除之前显示的录音文件名 */
                    recoder_enter_rec_mode(1024 * recagc);  /* 重新进入录音模式 */

                    while (vs10xx_read_reg(SPI_HDAT1) >> 8);/* 等到buf 较为空闲再开始 */

                    recoder_show_time(recsec);  /* 显示时间 */
                    recoder_show_agc(recagc);   /* 显示agc */
                }

                delay_ms(5);
                timecnt++;

                if ((timecnt % 20) == 0)LED0_TOGGLE();  /* DS0闪烁 */
            }

            if (recsec != (sectorsize * 4 / 125))   /* 录音时间显示 */
            {
                LED0_TOGGLE();              /* DS0闪烁 */
                recsec = sectorsize * 4 / 125;
                recoder_show_time(recsec);  /* 显示时间 */
            }
        }
    }

    myfree(SRAMIN, wavhead);
    myfree(SRAMIN, recbuf);
    myfree(SRAMIN, f_rec);
    myfree(SRAMIN, pname);
    return rval;
}



























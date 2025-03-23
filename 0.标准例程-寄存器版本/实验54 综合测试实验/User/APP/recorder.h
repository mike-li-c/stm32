/**
 ****************************************************************************************************
 * @file        recorder.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-10-28
 * @brief       APP-录音机 代码
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
 * 2, 修改uint8_t/uint16_t/uint32_t为uint8_t/uint16_t/uint32_t
 *
 ****************************************************************************************************
 */

#ifndef __RECORDER_H
#define __RECORDER_H

#include "common.h"


/* 各图标/图片路径 */
extern uint8_t *const RECORDER_DEMO_PIC;    /* demo图片路径 */
extern uint8_t *const RECORDER_RECR_PIC;    /* 录音 松开 */
extern uint8_t *const RECORDER_RECP_PIC;    /* 录音 按下 */
extern uint8_t *const RECORDER_PAUSER_PIC;  /* 暂停 松开 */
extern uint8_t *const RECORDER_PAUSEP_PIC;  /* 暂停 按下 */
extern uint8_t *const RECORDER_STOPR_PIC;   /* 停止 松开 */
extern uint8_t *const RECORDER_STOPP_PIC;   /* 停止 按下 */

/* RIFF块 */
typedef __packed struct
{
    uint32_t ChunkID;           /* chunk id;这里固定为"RIFF",即0X46464952 */
    uint32_t ChunkSize ;        /* 集合大小;文件总大小-8 */
    uint32_t Format;            /* 格式;WAVE,即0X45564157 */
} ChunkRIFF ;

/* fmt块 */
typedef __packed struct
{
    uint32_t ChunkID;           /* chunk id;这里固定为"fmt ",即0X20746D66 */
    uint32_t ChunkSize ;        /* 子集合大小(不包括ID和Size);这里为:20 */
    uint16_t AudioFormat;       /* 音频格式;0X10,表示线性PCM;0X11表示IMA ADPCM */
    uint16_t NumOfChannels;     /* 通道数量;1,表示单声道;2,表示双声道; */
    uint32_t SampleRate;        /* 采样率;0X1F40,表示8Khz */
    uint32_t ByteRate;          /* 字节速率; */
    uint16_t BlockAlign;        /* 块对齐(字节); */
    uint16_t BitsPerSample;     /* 单个采样数据大小;4位ADPCM,设置为4 */
    //uint16_t ByteExtraData;    /* 附加的数据字节;2个; 线性PCM,没有这个参数 */
    //uint16_t ExtraData;        /* 附加的数据,单个采样数据块大小;0X1F9:505字节  线性PCM,没有这个参数 */
} ChunkFMT;

/* fact块 */
typedef __packed struct
{
    uint32_t ChunkID;           /* chunk id;这里固定为"fact",即0X74636166; */
    uint32_t ChunkSize ;        /* 子集合大小(不包括ID和Size);这里为:4. */
    uint32_t NumOfSamples;      /* 采样的数量 */
} ChunkFACT;

/* data块 */
typedef __packed struct
{
    uint32_t ChunkID;           /* chunk id;这里固定为"data",即0X61746164 */
    uint32_t ChunkSize ;        /* 子集合大小(不包括ID和Size);文件大小-60 */
} ChunkDATA;

/* wav头 */
typedef __packed struct
{
    ChunkRIFF riff;             /* riff块 */
    ChunkFMT fmt;               /* fmt块 */
    //ChunkFACT fact;            /* fact块 线性PCM,没有这个结构体 */
    ChunkDATA data;             /* data块 */
} __WaveHeader;


void recorder_enter_rec_mode(void);
void recorder_wav_init(__WaveHeader *wavhead);/* 初始化WAV头 */
void recoder_set_agc(uint16_t agc);
uint16_t recorder_read_data(uint8_t address);
uint8_t recorder_vu_get(uint16_t signallevel);
void recorder_vu_meter(uint16_t x, uint16_t y, uint8_t level);
void recorder_show_time(uint16_t x, uint16_t y, uint32_t tsec);
void recorder_show_name(uint16_t x, uint16_t y, uint8_t *name);
void recorder_show_samplerate(uint16_t x, uint16_t y, uint16_t samplerate);
void recorder_load_ui(void);
void recorder_new_pathname(uint8_t *pname);
void recorder_show_agc(uint16_t x, uint16_t y, uint8_t agc);
uint8_t recorder_agc_set(uint16_t x, uint16_t y, uint8_t *agc, uint8_t *caption);
uint8_t recorder_play(void);

#endif
























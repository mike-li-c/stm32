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

#ifndef __RECORDER_H
#define __RECORDER_H


#include "./SYSTEM/sys/sys.h"

/* RIFF块 */
typedef __PACKED_STRUCT
{
    uint32_t ChunkID;           /* chunk id;这里固定为"RIFF",即0X46464952 */
    uint32_t ChunkSize ;        /* 集合大小;文件总大小-8 */
    uint32_t Format;            /* 格式;WAVE,即0X45564157 */
} ChunkRIFF ;

/* fmt块 */
typedef __PACKED_STRUCT
{
    uint32_t ChunkID;           /* chunk id;这里固定为"fmt ",即0X20746D66 */
    uint32_t ChunkSize ;        /* 子集合大小(不包括ID和Size);这里为:20 */
    uint16_t AudioFormat;       /* 音频格式;0X10,表示线性PCM;0X11表示IMA ADPCM */
    uint16_t NumOfChannels;     /* 通道数量;1,表示单声道;2,表示双声道; */
    uint32_t SampleRate;        /* 采样率;0X1F40,表示8Khz */
    uint32_t ByteRate;          /* 字节速率 */
    uint16_t BlockAlign;        /* 块对齐(字节 */
    uint16_t BitsPerSample;     /* 单个采样数据大小;4位ADPCM,设置为4 */
    //uint16_t ByteExtraData;   /* 附加的数据字节;2个; 线性PCM,没有这个参数 */
    //uint16_t ExtraData;       /* 附加的数据,单个采样数据块大小;0X1F9:505字节  线性PCM,没有这个参数 */
} ChunkFMT;

/* fact块 */
typedef __PACKED_STRUCT
{
    uint32_t ChunkID;           /* chunk id;这里固定为"fact",即0X74636166 */
    uint32_t ChunkSize ;        /* 子集合大小(不包括ID和Size);这里为:4 */
    uint32_t NumOfSamples;      /* 采样的数量 */
} ChunkFACT;

/* data块 */
typedef __PACKED_STRUCT
{
    uint32_t ChunkID;           /* chunk id;这里固定为"data",即0X61746164 */
    uint32_t ChunkSize ;        /* 子集合大小(不包括ID和Size);文件大小-60 */
} ChunkDATA;

/* wav头 */
typedef __PACKED_STRUCT
{
    ChunkRIFF riff;             /* riff块 */
    ChunkFMT fmt;               /* fmt块 */
    //ChunkFACT fact;           /* fact块 线性PCM,没有这个结构体 */
    ChunkDATA data;             /* data块 */
} __WaveHeader;

void recoder_enter_rec_mode(uint16_t agc);
void recoder_wav_init(__WaveHeader *wavhead);/* 初始化WAV头 */
void recoder_show_agc(uint8_t agc);
void recoder_show_time(uint32_t tsec);
uint8_t recoder_play(void);
#endif

















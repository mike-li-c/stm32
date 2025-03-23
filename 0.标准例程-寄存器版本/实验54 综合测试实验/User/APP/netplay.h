/**
 ****************************************************************************************************
 * @file        netplay.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-05-26
 * @brief       APP-网络测试 代码
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
 ****************************************************************************************************
 */

#ifndef __NETPLAY_H
#define __NETPLAY_H

#include "./BSP/CH395Q/ch395q.h"

#define NET_RECV_BUF_MAX_LEN    512     /* Socket数据接收缓冲区大小 */
#define NET_RMEMO_MAXLEN        10000   /* 接收区文本框文本最大长度 */
#define NET_SMEMO_MAXLEN        400     /* 发送区文本框文本最大长度 */
#define NET_TCP_SERVER_MAX_CONN 7       /* TCP Server模式下最大可连接数量，范围：1~7 */

#define NET_IP_BACK_COLOR       0x42F0  /* IP地址区域背景颜色 */
#define NET_MEMO_BACK_COLOR     0xA599  /* 分割线颜色 */
#define NET_MSG_FONT_COLOR      0x4A49  /* 提示消息字体颜色 */
#define NET_MSG_BACK_COLOR      0xBE3B  /* 提示消息背景颜色 */
#define NET_COM_RIM_COLOR       0x7BCF  /* 接收、发送区背景颜色 */

/* 函数声明 */
uint8_t net_play(void);

#endif

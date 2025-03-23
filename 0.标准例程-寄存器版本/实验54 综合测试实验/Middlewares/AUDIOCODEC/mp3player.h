/**
 ****************************************************************************************************
 * @file        mp3player.h
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

#ifndef __MP3PLAYER_H
#define __MP3PLAYER_H

#include "./SYSTEM/sys/sys.h"


uint32_t mp3_file_seek(uint32_t pos);
uint8_t mp3_play_song(uint8_t *pname); 

#endif













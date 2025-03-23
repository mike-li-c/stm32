/**
 ****************************************************************************************************
 * @file        joypad.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-26
 * @brief       游戏手柄 驱动代码
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
 * V1.0 20200426
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __JOYPAD_H
#define __JOYPAD_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 引脚 定义 */

#define JOYPAD_CLK_GPIO_PORT            GPIOD
#define JOYPAD_CLK_GPIO_PIN             SYS_GPIO_PIN3
#define JOYPAD_CLK_GPIO_CLK_ENABLE()    do{ RCC->APB2ENR |= 1 << 5; }while(0)   /* PD口时钟使能 */

#define JOYPAD_LAT_GPIO_PORT            GPIOB
#define JOYPAD_LAT_GPIO_PIN             SYS_GPIO_PIN11
#define JOYPAD_LAT_GPIO_CLK_ENABLE()    do{ RCC->APB2ENR |= 1 << 3; }while(0)   /* PB口时钟使能 */

#define JOYPAD_DATA_GPIO_PORT           GPIOB
#define JOYPAD_DATA_GPIO_PIN            SYS_GPIO_PIN10
#define JOYPAD_DATA_GPIO_CLK_ENABLE()   do{ RCC->APB2ENR |= 1 << 3; }while(0)   /* PB口时钟使能 */

/******************************************************************************************/

/* 手柄连接引脚 */
#define JOYPAD_CLK(x)   sys_gpio_pin_set(JOYPAD_CLK_GPIO_PORT, JOYPAD_CLK_GPIO_PIN, x)  /* JOYPAD_CLK */
#define JOYPAD_LAT(x)   sys_gpio_pin_set(JOYPAD_LAT_GPIO_PORT, JOYPAD_LAT_GPIO_PIN, x)  /* JOYPAD_LATCH */
#define JOYPAD_DATA     sys_gpio_pin_get(JOYPAD_DATA_GPIO_PORT, JOYPAD_DATA_GPIO_PIN)   /* JOYPAD_DATA */


/* 静态函数 */
static void joypad_delay(uint16_t t);   /* JOYPAD 延时 */

/* 接口函数 */
void joypad_init(void);     /* JOYPAD 初始化 */
uint8_t joypad_read(void);  /* JOYPAD 读取数据 */

#endif

















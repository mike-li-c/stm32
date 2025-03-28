/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-20
 * @brief       基本定时器中断 实验
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
 ****************************************************************************************************
 */

#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "btim.h"


int main(void)
{
    sys_stm32_clock_init(9);                /* 设置时钟, 72Mhz */
    delay_init(72);                         /* 延时初始化 */
    usart_init(72, 115200);                 /* 串口初始化为115200 */
    led_init();                             /* 初始化LED */
    btim_timx_int_init(5000 - 1, 7200 - 1); /* 10Khz的计数频率，计数5K次为500ms */

    while (1)
    {
        LED0_TOGGLE();
        delay_ms(200);
    }
}

/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-21
 * @brief       电容触摸按键 实验
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
#include "tpad.h"


int main(void)
{
    uint8_t t = 0;

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    led_init();                 /* 初始化LED */
    tpad_init(6);               /* 初始化触摸按键 */

    while (1)
    {
        if (tpad_scan(0))       /* 成功捕获到了一次上升沿(此函数执行时间至少15ms) */
        {
            LED1_TOGGLE();      /* LED1取反 */
        }

        t++;

        if (t == 15)
        {
            t = 0;
            LED0_TOGGLE();      /* LED0取反 */
        }

        delay_ms(10);
    }
}

















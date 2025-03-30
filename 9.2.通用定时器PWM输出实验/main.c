/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-20
 * @brief       通用定时器PWM输出 实验
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
#include "gtim.h"


int main(void)
{
    uint16_t ledrpwmval = 0;
    uint8_t dir = 1;
    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    led_init();                 /* 初始化LED */
    gtim_timx_pwm_chy_init(500 - 1, 72 - 1); /* 1Mhz的计数频率,2Khz的PWM. */

    while (1)
    {
        delay_ms(10);

        if (dir)ledrpwmval++;
        else ledrpwmval--;

        if (ledrpwmval > 300)dir = 0;
        if (ledrpwmval == 0)dir = 1;

        GTIM_TIMX_PWM_CHY_CCRX = ledrpwmval;
    }
}


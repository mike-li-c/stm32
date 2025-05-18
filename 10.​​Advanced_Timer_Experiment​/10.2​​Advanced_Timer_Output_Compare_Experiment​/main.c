/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-21
 * @brief       高级定时器输出比较模式 实验
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
#include "atim.h"

int main(void)
{
    uint8_t t = 0;

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    led_init();                 /* 初始化LED */

    atim_timx_comp_pwm_init(1000 - 1, 72 - 1);  /* 1Mhz的计数频率 1Khz的周期. */

    ATIM_TIMX_COMP_CH1_CCRX = 250 - 1;  /* 通道1 相位25% */
    ATIM_TIMX_COMP_CH2_CCRX = 500 - 1;  /* 通道2 相位50% */
    ATIM_TIMX_COMP_CH3_CCRX = 750 - 1;  /* 通道3 相位75% */
    ATIM_TIMX_COMP_CH4_CCRX = 1000 - 1; /* 通道4 相位100% */

    while (1)
    {
        delay_ms(10);
        t++;

        if (t >= 20)
        {
            LED0_TOGGLE();  /* LED0(RED)闪烁 */
            t = 0;
        }
    }
}

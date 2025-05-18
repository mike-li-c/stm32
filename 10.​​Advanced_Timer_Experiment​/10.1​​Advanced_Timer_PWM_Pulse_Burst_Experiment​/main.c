/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-21
 * @brief       高级定时器输出指定个数PWM 实验
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
#include "key.h"
#include "atim.h"

int main(void)
{
    uint8_t key = 0;
    uint8_t t = 0;

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    led_init();                 /* 初始化LED */
    key_init();                 /* 初始化按键 */

    /* 将 LED1 引脚设置为输入模式, 避免和PC6冲突 */
    sys_gpio_set(LED1_GPIO_PORT, LED1_GPIO_PIN,
                 SYS_GPIO_MODE_IN, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_MID, SYS_GPIO_PUPD_PU);

    atim_timx_npwm_chy_init(5000 - 1, 7200 - 1);    /* 10Khz的计数频率,2hz的PWM频率. */

    ATIM_TIMX_NPWM_CHY_CCRX = 2500; /* 设置PWM占空比,50%,这样可以控制每一个PWM周期,LED2(BLUE)
                                     * 有一半时间是亮的,一半时间是灭的,LED2亮灭一次,表示一个PWM波
                                     */
    atim_timx_npwm_chy_set(5);  /* 输出5个PWM波(控制LED2(BLUE)闪烁5次) */

    while (1)
    {
        key = key_scan(0);

        if (key == KEY0_PRES)   /* KEY0按下 */
        {
            atim_timx_npwm_chy_set(5);  /* 输出5个PWM波(控制TIM8_CH1, 即PC6输出5个脉冲) */
        }

        t++;
        delay_ms(10);
        
        if (t > 50)             /* 控制LED0闪烁, 提示程序运行状态 */
        {
            t = 0;
            LED0_TOGGLE();
        }
    }
}

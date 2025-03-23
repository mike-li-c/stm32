/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-20
 * @brief       独立看门狗 实验
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

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/WDG/wdg.h"
#include "./BSP/KEY/key.h"


int main(void)
{
    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    led_init();                 /* 初始化LED */
    key_init();                 /* 初始化按键 */
    delay_ms(100);              /* 延时100ms再初始化看门狗,LED0的变化"可见" */
    iwdg_init(4, 625);          /* 预分频数为64,重载值为625,溢出时间为1s */
    LED0(0);                    /* 点亮LED0(红灯) */

    while (1)
    {
        if (key_scan(1) == WKUP_PRES) /* 如果WK_UP按下,则喂狗 */
        {
            iwdg_feed();    /* 喂狗 */
        }

        delay_ms(10);
    }
}


















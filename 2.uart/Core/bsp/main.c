/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-19
 * @brief       串口通信 实验
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


int main(void)
{
    uint8_t t;
    uint8_t len;
    uint16_t times = 0;

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    led_init();                 /* 初始化LED */

    while (1)
    {
        if (g_usart_rx_sta & 0x8000)        /* 接收到了数据? */
        {
            len = g_usart_rx_sta & 0x3fff;  /* 得到此次接收到的数据长度 */
            printf("the message you send is:");
            for (t = 0; t < len; t++)
            {
                USART_UX->DR = g_usart_rx_buf[t];
                while ((USART_UX->SR & 0X40) == 0); /* 等待发送结束 */
            }

            g_usart_rx_sta = 0;
        }
        else
        {
            times++;

            // if (times % 5000 == 0)
            // {
            //     printf("uart experiment");
            // }

            // if (times % 200 == 0) printf("please input message");

            if (times % 30  == 0) LED0_TOGGLE(); /* 闪烁LED,提示系统正在运行. */

            delay_ms(10);
        }
    }
}






















/**
 ****************************************************************************************************
 * @file        main.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-19
 * @brief       ����ͨ�� ʵ��
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� STM32F103������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
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

    sys_stm32_clock_init(9);    /* ����ʱ��, 72Mhz */
    delay_init(72);             /* ��ʱ��ʼ�� */
    usart_init(72, 115200);     /* ���ڳ�ʼ��Ϊ115200 */
    led_init();                 /* ��ʼ��LED */

    while (1)
    {
        if (g_usart_rx_sta & 0x8000)        /* ���յ�������? */
        {
            len = g_usart_rx_sta & 0x3fff;  /* �õ��˴ν��յ������ݳ��� */
            printf("\n�����͵���ϢΪ:\n");

            for (t = 0; t < len; t++)
            {
                USART_UX->DR = g_usart_rx_buf[t];
                
                while ((USART_UX->SR & 0X40) == 0); /* �ȴ����ͽ��� */
            }

            printf("\n\n"); /* ���뻻�� */
            g_usart_rx_sta = 0;
        }
        else
        {
            times++;

            if (times % 5000 == 0)
            {
                printf("\nuart experience\n");
                printf("����ԭ��@ALIENTEK\n\n\n");
            }

            if (times % 200 == 0) printf("����������,�Իس�������\n");

            if (times % 30  == 0) LED0_TOGGLE(); /* ��˸LED,��ʾϵͳ��������. */

            delay_ms(10);
        }
    }
}






















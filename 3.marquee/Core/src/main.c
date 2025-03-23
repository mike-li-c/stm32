/**
 ****************************************************************************************************
 * @file        main.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-17
 * @brief       ����� ʵ��
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
// #include "usart.h"
#include "delay.h"
#include "led.h"


int main(void)
{
    sys_stm32_clock_init(9);    /* ����ʱ��, 72Mhz */
    delay_init(72);             /* ��ʱ��ʼ�� */
    led_init();                 /* ��ʼ��LED */

    while (1)
    {
        LED0(0);                /* LED0 �� */
        LED1(1);                /* LED1 �� */
        delay_ms(1000);
        // LED0_TOGGLE();             /* LED0 �� */
        // LED1_TOGGLE();                /* LED1 �� */
                LED0_TOGGLE();             /* LED0 �� */
        LED1_TOGGLE();                /* LED1 �� */
        delay_ms(1000);
    }
}

















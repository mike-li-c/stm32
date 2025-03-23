/**
 ****************************************************************************************************
 * @file        pwmdac.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-24
 * @brief       PWM DAC��� ��������
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
 * �޸�˵��
 * V1.0 20200424
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#include "./BSP/PWMDAC/pwmdac.h"

/**
 * @brief       PWM DAC��ʼ��, ʵ���Ͼ��ǳ�ʼ����ʱ��
 * @note
 *              ��ʱ����ʱ������APB1 / APB2, ��APB1 / APB2 ��Ƶʱ, ��ʱ��Ƶ���Զ�����
 *              ����, һ�������, �������ж�ʱ����Ƶ��, ����72Mhz ����ϵͳʱ��Ƶ��
 *              ��ʱ�����ʱ����㷽��: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft = ��ʱ������Ƶ��, ��λ: Mhz
 *
 * @param       arr: �Զ���װֵ��
 * @param       psc: ʱ��Ԥ��Ƶ��
 * @retval      ��
 */
void pwmdac_init(uint16_t arr, uint16_t psc)
{
    uint8_t chy = PWMDAC_TIMX_CHY;
    PWMDAC_GPIO_CLK_ENABLE();       /* PWM DAC GPIO ʱ��ʹ�� */
    PWMDAC_TIMX_CLK_ENABLE();       /* PWM DAC ��ʱ��ʱ��ʹ�� */

    sys_gpio_set(PWMDAC_GPIO_PORT, PWMDAC_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* TIMX PWM CHY ����ģʽ���� */

    PWMDAC_TIMX->ARR = arr;         /* �趨�������Զ���װֵ */
    PWMDAC_TIMX->PSC = psc;         /* ����Ԥ��Ƶ��  */
    PWMDAC_TIMX->BDTR |= 1 << 15;   /* ʹ��MOEλ(��TIM1/8 �д˼Ĵ���,��������MOE�������PWM), ����ͨ�ö�ʱ��, ���
                                     * �Ĵ�������Ч��, ��������/�����ò���Ӱ����, Ϊ�˼�������ͳһ�ĳ�����MOEλ
                                     */

    if (chy <= 2)
    {
        PWMDAC_TIMX->CCMR1 |= 6 << (4 + 8 * (chy - 1)); /* CH1/2 PWMģʽ1 */
        PWMDAC_TIMX->CCMR1 |= 1 << (3 + 8 * (chy - 1)); /* CH1/2 Ԥװ��ʹ�� */
    }
    else if (chy <= 4)
    {
        PWMDAC_TIMX->CCMR2 |= 6 << (4 + 8 * (chy - 3)); /* CH3/4 PWMģʽ1 */
        PWMDAC_TIMX->CCMR2 |= 1 << (3 + 8 * (chy - 3)); /* CH3/4 Ԥװ��ʹ�� */
    }

    PWMDAC_TIMX->CCER |= 1 << (4 * (chy - 1));      /* OCy ���ʹ�� */
    PWMDAC_TIMX->CCER |= 0 << (1 + 4 * (chy - 1));  /* OCy �ߵ�ƽ��Ч */
    PWMDAC_TIMX->CR1 |= 1 << 7;     /* ARPEʹ�� */
    PWMDAC_TIMX->CR1 |= 1 << 0;     /* ʹ�ܶ�ʱ��TIMX */
}

/**
 * @brief       ����PWM DAC�����ѹ
 * @param       vol : 0~3300,����0~3.3V
 * @retval      ��
 */
void pwmdac_set_voltage(uint16_t vol)
{
    float temp = vol;
    temp /= 100;                /* ��С100��, �õ�ʵ�ʵ�ѹֵ */
    temp = temp * 256 / 3.3;    /* ����ѹת����PWMռ�ձ� */
    PWMDAC_TIMX_CCRX = temp;    /* �����µ�ռ�ձ� */
}









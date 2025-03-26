/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-17
 * @brief       ���ڳ�ʼ������(һ���Ǵ���1)��֧��printf
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
 * V1.0 20200417
 * ��һ�η���
 *
 ****************************************************************************************************
 */
 
#include "sys.h"
#include "usart.h"
#include <stdio.h>
/* ���ʹ��os,����������ͷ�ļ�����. */
#if SYS_SUPPORT_OS
#include "os.h"   /* os ʹ�� */
#endif



/******************************************************************************************/
/* �������´���, ֧��printf����*/
#ifdef __GNUC__
int _write(int fd, char *ptr, int len)  
{  
    for (int t = 0; t < len; t++)
    {
        USART_UX->DR = ptr[t];
        while ((USART_UX->SR & 0X40) == 0); /* �ȴ����ͽ��� */
    }
  return len;
}

#endif
/******************************************************************************************/


#if USART_EN_RX     /* ���ʹ���˽��� */

/* ���ջ���, ���USART_REC_LEN���ֽ�. */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/*  ����״̬
 *  bit15��      ������ɱ�־
 *  bit14��      ���յ�0x0d
 *  bit13~0��    ���յ�����Ч�ֽ���Ŀ
*/
uint16_t g_usart_rx_sta = 0;

/**
 * @brief       ����X�жϷ�����
 * @param       ��
 * @retval      ��
 */
void USART_UX_IRQHandler(void)
{
    uint8_t rxdata;
 
    if (USART_UX->SR & (1 << 5))                /* ���յ����� */
    {
        rxdata = USART_UX->DR;
            /* ����δ���? */
        if (rxdata == 10)    /*10 = 0x0d = \n, 10��\n��ascii��*/
        {

            g_usart_rx_sta |= 0x8000;   /* ���յ�\n, ��ǽ�������� */
        }
        else      /* ��û�յ�\n*/
        {
            g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = rxdata;   /* �洢���ݵ� g_usart_rx_buf */
            g_usart_rx_sta++;

            if (g_usart_rx_sta > (USART_REC_LEN - 1))g_usart_rx_sta = 0;/* �����������, ���¿�ʼ���� */
        }
    }
}

#endif

/**
 * @brief       ����X��ʼ������
 * @param       sclk: ����X��ʱ��ԴƵ��(��λ: MHz)
 *              ����1 ��ʱ��Դ����: PCLK2 = 72Mhz
 *              ����2 - 5 ��ʱ��Դ����: PCLK1 = 36Mhz
 * @note        ע��: ����������ȷ��sclk, ���򴮿ڲ����ʾͻ������쳣.
 * @param       baudrate: ������, �����Լ���Ҫ���ò�����ֵ
 * @retval      ��
 */
void usart_init(uint32_t sclk, uint32_t baudrate)
{
    uint32_t temp;
    /* IO �� ʱ������ */
    USART_TX_GPIO_CLK_ENABLE(); /* ʹ�ܴ���TX��ʱ�� */
    USART_RX_GPIO_CLK_ENABLE(); /* ʹ�ܴ���RX��ʱ�� */
    USART_UX_CLK_ENABLE();      /* ʹ�ܴ���ʱ�� */

    sys_gpio_set(USART_TX_GPIO_PORT, USART_TX_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* ����TX�� ģʽ���� */

    sys_gpio_set(USART_RX_GPIO_PORT, USART_RX_GPIO_PIN,
                 SYS_GPIO_MODE_IN, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* ����RX�� �������ó�����ģʽ */

    temp = (sclk * 1000000 + baudrate / 2) / baudrate;  /* �õ�BRR, ��������������� */
    /* ���������� */
    USART_UX->BRR = temp;       /* ���������� */
    USART_UX->CR1 = 0;          /* ����CR1�Ĵ��� */
    USART_UX->CR1 |= 0 << 12;   /* M = 0, 1����ʼλ, 8������λ, n��ֹͣλ(��USART_CR2 STOP[1:0]ָ��, Ĭ����0, ��ʾ1��ֹͣλ) */
    USART_UX->CR1 |= 1 << 3;    /* TE = 1, ���ڷ���ʹ�� */
#if USART_EN_RX  /* ���ʹ���˽��� */
    /* ʹ�ܽ����ж� */
    USART_UX->CR1 |= 1 << 2;    /* RE = 1, ���ڽ���ʹ�� */
    USART_UX->CR1 |= 1 << 5;    /* RXNEIE = 1, ���ջ������ǿ��ж�ʹ�� */
    sys_nvic_init(3, 3, USART_UX_IRQn, 2); /* ��2��������ȼ� */
#endif
    USART_UX->CR1 |= 1 << 13;   /* UE = 1, ����ʹ�� */
}















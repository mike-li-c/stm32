/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-17
 * @brief       串口初始化代码(一般是串口1)，支持printf
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
 * 修改说明
 * V1.0 20200417
 * 第一次发布
 *
 ****************************************************************************************************
 */
 
#include "sys.h"
#include "usart.h"
#include <stdio.h>
/* 如果使用os,则包括下面的头文件即可. */
#if SYS_SUPPORT_OS
#include "os.h"   /* os 使用 */
#endif



/******************************************************************************************/
/* 加入以下代码, 支持printf函数*/
#ifdef __GNUC__
/* gcc重定义_write函数, printf函数最终会通过调用_write输出字符串到串口，与MDK不同 */
int _write (int fd, char *pBuffer, int size)  
{  
    for (int i = 0; i < size; i++)  
    {  
        while((USART1->SR&0X40)==0);//等待上一次串口数据发送完成  
        USART1->DR = (u8) pBuffer;       //写DR,串口1将发送数据
    }  
    return size;  
}
#endif
/******************************************************************************************/


#if USART_EN_RX     /* 如果使能了接收 */

/* 接收缓冲, 最大USART_REC_LEN个字节. */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart_rx_sta = 0;

/**
 * @brief       串口X中断服务函数
 * @param       无
 * @retval      无
 */
void USART_UX_IRQHandler(void)
{
    uint8_t rxdata;
#if SYS_SUPPORT_OS  /* 如果SYS_SUPPORT_OS为真，则需要支持OS. */
    OSIntEnter();
#endif

    if (USART_UX->SR & (1 << 5))                /* 接收到数据 */
    {
        rxdata = USART_UX->DR;

        if ((g_usart_rx_sta & 0x8000) == 0)     /* 接收未完成? */
        {
            if (g_usart_rx_sta & 0x4000)        /* 接收到了0x0d? */
            {
                if (rxdata != 0x0a)             /* 接收到了0x0a? (必须先接收到到0x0d,才检查0x0a) */
                {
                    g_usart_rx_sta = 0;         /* 接收错误, 重新开始 */
                }
                else
                {
                    g_usart_rx_sta |= 0x8000;   /* 收到了0x0a,标记接收完成了 */
                }
            }
            else      /* 还没收到0x0d */
            {
                if (rxdata == 0x0d)
                {
                    g_usart_rx_sta |= 0x4000;   /* 标记接收到了 0x0d */
                }
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = rxdata;   /* 存储数据到 g_usart_rx_buf */
                    g_usart_rx_sta++;

                    if (g_usart_rx_sta > (USART_REC_LEN - 1))g_usart_rx_sta = 0;/* 接收数据溢出, 重新开始接收 */
                }
            }
        }
    }

#if SYS_SUPPORT_OS  /* 如果SYS_SUPPORT_OS为真，则需要支持OS. */
    OSIntExit();
#endif
}
#endif

/**
 * @brief       串口X初始化函数
 * @param       sclk: 串口X的时钟源频率(单位: MHz)
 *              串口1 的时钟源来自: PCLK2 = 72Mhz
 *              串口2 - 5 的时钟源来自: PCLK1 = 36Mhz
 * @note        注意: 必须设置正确的sclk, 否则串口波特率就会设置异常.
 * @param       baudrate: 波特率, 根据自己需要设置波特率值
 * @retval      无
 */
void usart_init(uint32_t sclk, uint32_t baudrate)
{
    uint32_t temp;
    /* IO 及 时钟配置 */
    USART_TX_GPIO_CLK_ENABLE(); /* 使能串口TX脚时钟 */
    USART_RX_GPIO_CLK_ENABLE(); /* 使能串口RX脚时钟 */
    USART_UX_CLK_ENABLE();      /* 使能串口时钟 */

    sys_gpio_set(USART_TX_GPIO_PORT, USART_TX_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* 串口TX脚 模式设置 */

    sys_gpio_set(USART_RX_GPIO_PORT, USART_RX_GPIO_PIN,
                 SYS_GPIO_MODE_IN, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* 串口RX脚 必须设置成输入模式 */

    temp = (sclk * 1000000 + baudrate / 2) / baudrate;  /* 得到BRR, 采用四舍五入计算 */
    /* 波特率设置 */
    USART_UX->BRR = temp;       /* 波特率设置 */
    USART_UX->CR1 = 0;          /* 清零CR1寄存器 */
    USART_UX->CR1 |= 0 << 12;   /* M = 0, 1个起始位, 8个数据位, n个停止位(由USART_CR2 STOP[1:0]指定, 默认是0, 表示1个停止位) */
    USART_UX->CR1 |= 1 << 3;    /* TE = 1, 串口发送使能 */
#if USART_EN_RX  /* 如果使能了接收 */
    /* 使能接收中断 */
    USART_UX->CR1 |= 1 << 2;    /* RE = 1, 串口接收使能 */
    USART_UX->CR1 |= 1 << 5;    /* RXNEIE = 1, 接收缓冲区非空中断使能 */
    sys_nvic_init(3, 3, USART_UX_IRQn, 2); /* 组2，最低优先级 */
#endif
    USART_UX->CR1 |= 1 << 13;   /* UE = 1, 串口使能 */
}















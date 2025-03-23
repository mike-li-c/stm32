/**
 ****************************************************************************************************
 * @file        usart3.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-10-29
 * @brief       串口3 驱动代码
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
 * V1.0 20221029
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/USART3/usart3.h"
#include "./BSP/TIMER/timer.h"
#include "./MALLOC/malloc.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ucos_ii.h"


/* 串口接收缓存区 */
uint8_t g_usart3_rx_buf[USART3_MAX_RECV_LEN];       /* 接收缓冲,最大USART3_MAX_RECV_LEN个字节 */


/**
 * 通过判断接收连续2个字符之间的时间差不大于10ms来决定是不是一次连续的数据.
 * 如果2个字符接收间隔超过10ms,则认为不是1次连续数据.也就是超过10ms没有接收到
 * 任何数据,则表示此次接收完毕.
 * 接收到的数据状态
 * [15]:0,没有接收到数据;1,接收到了一批数据.
 * [14:0]:接收到的数据长度
 */
volatile uint16_t g_usart3_rx_sta = 0;

/**
 * @brief       串口3中断服务函数
 * @param       无
 * @retval      无
 */
void USART3_IRQHandler(void)
{
    uint8_t res;
    OSIntEnter();

    if (USART3->SR & (1 << 5))  /* 接收到数据 */
    {
        res = USART3->DR;

        if ((g_usart3_rx_sta & (1 << 15)) == 0) /* 接收完的一批数据,还没有被处理,则不再接收其他数据 */
        {
            if (g_usart3_rx_sta < USART3_MAX_RECV_LEN)      /* 还可以接收数据 */
            {
                TIM7->CNT = 0;                  /* 计数器清空 */

                if (g_usart3_rx_sta == 0)       /* 使能定时器7的中断 */
                {
                    TIM7->CR1 |= 1 << 0;        /* 使能定时器7 */
                }

                g_usart3_rx_buf[g_usart3_rx_sta++] = res;   /* 记录接收到的值 */
            }
            else
            {
                g_usart3_rx_sta |= 1 << 15;     /* 强制标记接收完成 */
            }
        }
    }

    OSIntExit();
}

/**
 * @brief       串口3初始化函数
 * @param       sclk: 串口X的时钟源频率(单位: MHz)
 *              串口1 的时钟源来自: PCLK2 = 72Mhz
 *              串口2 - 5 的时钟源来自: PCLK1 = 36Mhz
 * @note        注意: 必须设置正确的sclk, 否则串口波特率就会设置异常.
 * @param       baudrate: 波特率, 根据自己需要设置波特率值
 * @retval      无
 */
void usart3_init(uint32_t sclk, uint32_t baudrate)
{
    USART3_TX_GPIO_CLK_ENABLE();    /* 使能串口TX脚时钟 */
    USART3_RX_GPIO_CLK_ENABLE();    /* 使能串口RX脚时钟 */
    
    sys_gpio_set(USART3_TX_GPIO_PORT, USART3_TX_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* 串口TX脚 模式设置 */

    sys_gpio_set(USART3_RX_GPIO_PORT, USART3_RX_GPIO_PIN,
                 SYS_GPIO_MODE_IN, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* 串口RX脚 必须设置成输入模式 */

    RCC->APB1ENR |= 1 << 18;        /* 使能串口3时钟 */
    RCC->APB1RSTR |= 1 << 18;       /* 复位串口3 */
    RCC->APB1RSTR &= ~(1 << 18);    /* 停止复位 */
    
    /* 波特率设置 */
    USART3->BRR = (sclk * 1000000) / (baudrate); /*  波特率设置 */
    USART3->CR1 |= 0X200C;          /* 1位停止,无校验位 */
    
    /* 使能接收中断 */
    USART3->CR1 |= 1 << 5;          /* 接收缓冲区非空中断使能 */
    
    sys_nvic_init(0, 1, USART3_IRQn, 2); /* 组2 */
    
    tim7_int_init(99, 7199);        /* 10ms中断 */
    TIM7->CR1 &= ~(1 << 0);         /* 关闭定时器7 */
    g_usart3_rx_sta = 0;            /* 清零 */
}

/**
 * @brief       串口3,printf 函数
 * @param       fmt     : 格式化参数
 * @param       ...     : 可变参数
 * @retval      无
 */
void u3_printf(char *fmt, ...)
{
    uint16_t i, j;
    uint8_t *pbuf;
    va_list ap;
    
    pbuf = mymalloc(SRAMIN, USART3_MAX_SEND_LEN);   /* 申请内存 */

    if (!pbuf)  /* 内存申请失败 */
    {
        printf("u3 malloc error\r\n");
        return ;
    }

    va_start(ap, fmt);
    vsprintf((char *)pbuf, fmt, ap);
    va_end(ap);
    i = strlen((const char *)pbuf); /* 此次发送数据的长度 */

    for (j = 0; j < i; j++)         /* 循环发送数据 */
    {
        while ((USART3->SR & 0X40) == 0);   /* 循环发送,直到发送完毕 */

        USART3->DR = pbuf[j];
    }

    myfree(SRAMIN, pbuf);           /* 释放内存 */
}




































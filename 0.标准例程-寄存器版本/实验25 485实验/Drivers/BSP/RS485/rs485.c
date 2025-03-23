/**
 ****************************************************************************************************
 * @file        rs485.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-24
 * @brief       RS485 驱动代码
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
 * V1.0 20200424
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/RS485/rs485.h"
#include "./SYSTEM/delay/delay.h"


#ifdef RS485_EN_RX      /* 如果使能了接收 */

uint8_t g_RS485_rx_buf[RS485_REC_LEN];  /* 接收缓冲, 最大 RS485_REC_LEN 个字节. */
uint8_t g_RS485_rx_cnt = 0;             /* 接收到的数据长度 */

void RS485_UX_IRQHandler(void)
{
    uint8_t res;

    if (RS485_UX->SR & (1 << 5))    /* 接收到数据 */
    {
        res = RS485_UX->DR;

        if (g_RS485_rx_cnt < RS485_REC_LEN)         /* 缓冲区未满 */
        {
            g_RS485_rx_buf[g_RS485_rx_cnt] = res;   /* 记录接收到的值 */
            g_RS485_rx_cnt++;                       /* 接收数据增加1 */
        }
    }
}

#endif

/**
 * @brief       RS485初始化函数
 *   @note      该函数主要是初始化串口
 * @param       sclk    : 串口X的时钟源频率(单位: MHz)
 *              串口1 的时钟源来自: PCLK2 = 72Mhz
 *              串口2 - 5 的时钟源来自: PCLK1 = 36Mhz
 * @note        注意: 必须设置正确的sclk, 否则串口波特率就会设置异常.
 * @param       baudrate: 波特率, 根据自己需要设置波特率值
 * @retval      无
 */
void rs485_init(uint32_t sclk, uint32_t baudrate)
{
    uint32_t temp;

    /* IO 及 时钟配置 */
    RS485_RE_GPIO_CLK_ENABLE(); /* 使能 RS485_RE 脚时钟 */
    RS485_TX_GPIO_CLK_ENABLE(); /* 使能 串口TX脚 时钟 */
    RS485_RX_GPIO_CLK_ENABLE(); /* 使能 串口RX脚 时钟 */
    RS485_UX_CLK_ENABLE();      /* 使能 串口 时钟 */

    sys_gpio_set(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN,
                 SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);  /* RS485_RE 脚 模式设置 */

    sys_gpio_set(RS485_TX_GPIO_PORT, RS485_TX_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* 串口TX 脚 模式设置 */

    sys_gpio_set(RS485_RX_GPIO_PORT, RS485_RX_GPIO_PIN,
                 SYS_GPIO_MODE_IN, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* 串口RX 脚 必须设置成输入模式 */

    temp = (sclk * 1000000 + baudrate / 2) / baudrate;  /* 得到BRR, 采用四舍五入计算 */
    
    /* 波特率设置 */
    RS485_UX->BRR = temp;       /* 波特率设置 */
    RS485_UX->CR1 = 0;          /* 清零CR1寄存器 */
    RS485_UX->CR1 |= 0 << 12;   /* M = 0, 1个起始位, 8个数据位, n个停止位(由USART_CR2 STOP[1:0]指定, 默认是0, 表示1个停止位) */
    RS485_UX->CR1 |= 1 << 3;    /* TE = 1, 串口发送使能 */
#if RS485_EN_RX  /* 如果使能了接收 */
    /* 使能接收中断 */
    RS485_UX->CR1 |= 1 << 2;    /* RE = 1, 串口接收使能 */
    RS485_UX->CR1 |= 1 << 5;    /* RXNEIE = 1, 接收缓冲区非空中断使能 */
    sys_nvic_init(3, 3, RS485_UX_IRQn, 2); /* 组2，最低优先级 */
#endif
    RS485_UX->CR1 |= 1 << 13;   /* UE = 1, 串口使能 */

    RS485_RE(0);                /* 默认为接收模式 */
}

/**
 * @brief       RS485发送len个字节
 * @param       buf     : 发送区首地址
 * @param       len     : 发送的字节数(为了和本代码的接收匹配,这里建议不要超过 RS485_REC_LEN 个字节)
 * @retval      无
 */
void rs485_send_data(uint8_t *buf, uint8_t len)
{
    uint8_t t;
    RS485_RE(1);    /* 进入发送模式 */

    for (t = 0; t < len; t++)   /* 循环发送数据 */
    {
        while ((RS485_UX->SR & 0X40) == 0); /* 等待发送结束 */

        RS485_UX->DR = buf[t];
    }

    while ((RS485_UX->SR & 0X40) == 0);     /* 等待发送结束 */

    g_RS485_rx_cnt = 0;
    RS485_RE(0);    /* 进入接收模式 */
}

/**
 * @brief       RS485查询接收到的数据
 * @param       buf     : 接收缓冲区首地址
 * @param       len     : 接收到的数据长度
 *   @arg               0   , 表示没有接收到任何数据
 *   @arg               其他, 表示接收到的数据长度
 * @retval      无
 */
void rs485_receive_data(uint8_t *buf, uint8_t *len)
{
    uint8_t rxlen = g_RS485_rx_cnt;
    uint8_t i = 0;
    *len = 0;       /* 默认为0 */
    delay_ms(10);   /* 等待10ms,连续超过10ms没有接收到一个数据,则认为接收结束 */

    if (rxlen == g_RS485_rx_cnt && rxlen)   /* 接收到了数据,且接收完成了 */
    {
        for (i = 0; i < rxlen; i++)
        {
            buf[i] = g_RS485_rx_buf[i];
        }

        *len = g_RS485_rx_cnt;  /* 记录本次数据长度 */
        g_RS485_rx_cnt = 0;     /* 清零 */
    }
}















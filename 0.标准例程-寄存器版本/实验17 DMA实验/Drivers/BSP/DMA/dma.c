/**
 ****************************************************************************************************
 * @file        dma.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-23
 * @brief       DMA 驱动代码
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
 * V1.0 20200423
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/DMA/dma.h"
#include "./SYSTEM/delay/delay.h"


/**
 * @brief       串口1 TX DMA初始化函数
 * @param       mar         : 存储器地址
 * @retval      无
 */
void dma_usart1_tx_config(uint32_t mar)
{
    DMA_Channel_TypeDef *dmax_chy;
    
    dmax_chy = DMA1_Channel4;   /* USART1_TX使用的DMA通道为: DMA1_Channel4 */

    /* 使能DMA时钟, 并设置CPAR和CMAR寄存器, 这里外设地址为: &USART1->DR */
    dma_basic_config(dmax_chy, (uint32_t)&USART1->DR, mar);

    dmax_chy->CCR = 0X00000000; /* 复位CCR寄存器 */
    dmax_chy->CCR |= 1 << 4;    /* DIR = 1 , 存储器到外设模式 */
    dmax_chy->CCR |= 0 << 5;    /* CIRC = 0, 非循环模式(即使用普通模式) */
    dmax_chy->CCR |= 0 << 6;    /* PINC = 0, 外设地址非增量模式 */
    dmax_chy->CCR |= 1 << 7;    /* MINC = 1, 存储器增量模式 */
    dmax_chy->CCR |= 0 << 8;    /* PSIZE[1:0] = 0, 外设数据宽度为: 8位 */
    dmax_chy->CCR |= 0 << 10;   /* MSIZE[1:0] = 0, 存储器数据宽度: 8位 */
    dmax_chy->CCR |= 1 << 12;   /* PL[1:0] =  1, 中等优先级 */
    dmax_chy->CCR |= 0 << 14;   /* MEM2MEM = 0 , 非存储器到存储器模式 */
}

/**
 * @brief       DMA基本配置
 *   @note      这里仅对DMA完成一些基础性的配置, 包括: DMA时钟使能 / 设置外设地址 和 存储器地址
 *              其他配置参数(CCR寄存器), 需用户自己另外实现
 *
 * @param       dmax_chy    : DMA及通道, DMA1_Channel1 ~ DMA1_Channel7, DMA2_Channel1 ~ DMA2_Channel5
 *                            某个外设对应哪个DMA, 哪个通道, 请参考<<STM32中文参考手册 V10>> 10.3.7节
 *                            必须设置正确的DMA及通道, 才能正常使用! 
 * @param       par         : 外设地址
 * @param       mar         : 存储器地址
 * @retval      无
 */
void dma_basic_config(DMA_Channel_TypeDef *dmax_chy,  uint32_t par, uint32_t mar)
{
    if (dmax_chy > DMA1_Channel7)   /* 大于DMA1_Channel7, 则为DMA2的通道了 */
    {
        RCC->AHBENR |= 1 << 1;  /* 开启DMA2时钟 */
    }
    else
    {
        RCC->AHBENR |= 1 << 0;  /* 开启DMA1时钟 */
    }

    delay_ms(5);                /* 等待DMA时钟稳定 */

    dmax_chy->CPAR = par;       /* DMA 外设地址 */
    dmax_chy->CMAR = mar;       /* DMA 存储器地址 */
    dmax_chy->CNDTR = 0;        /* DMA 传输长度清零, 后续在dma_enable函数设置 */
}


/**
 * @brief       开启一次DMA传输
 * @param       dmax_chy    : DMA及通道, DMA1_Channel1 ~ DMA1_Channel7, DMA2_Channel1 ~ DMA2_Channel5
 *                            某个外设对应哪个DMA, 哪个通道, 请参考<<STM32中文参考手册 V10>> 10.3.7节
 *                            必须设置正确的DMA及通道, 才能正常使用! 
 * @param       cndtr       : 数据传输量
 * @retval      无
 */
void dma_enable(DMA_Channel_TypeDef *dmax_chy, uint16_t cndtr)
{
    dmax_chy->CCR &= ~(1 << 0); /* 关闭DMA传输 */

    while (dmax_chy->CCR & (1 << 0));   /* 确保DMA可以被设置 */

    dmax_chy->CNDTR = cndtr;    /* DMA传输数据量 */
    dmax_chy->CCR |= 1 << 0;    /* 开启DMA传输 */
}




























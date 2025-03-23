/**
 ****************************************************************************************************
 * @file        spblcd.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-10-26
 * @brief       SPB效果实现 驱动代码
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
 * V1.0 20221026
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/SPBLCD/spblcd.h"
#include "./BSP/DMA/dma.h"
#include "./MALLOC/malloc.h"
#include "spb.h"


uint16_t *g_sramlcdbuf; /* SRAM LCD BUFFER,背景图片显存区 */

/**
 * @brief       在指定位置画点
 * @param       x,y     : 点坐标
 * @param       color   : 点颜色
 * @retval      无
 */
void slcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    g_sramlcdbuf[y + x * spbdev.spbheight] = color;
}

/**
 * @brief       读取指定位置点的颜色值
 * @param       x,y     : 点坐标
 * @retval      颜色值
 */
uint16_t slcd_read_point(uint16_t x, uint16_t y)
{
    return g_sramlcdbuf[y + x * spbdev.spbheight];
}

/**
 * @brief       填充颜色
 * @param       x,y     : 点坐标
 * @param       width   : 宽度
 * @param       height  : 高度
 * @param       *color  : 颜色数组
 * @retval      无
 */
void slcd_fill_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *color)
{
    uint16_t i, j;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            slcd_draw_point(x + j, y + i, *color++);
        }
    }
}

/**
 * @brief       SRAM到LCD DMA配置
 *              16位,外部SRAM传输到LCD GRAM
 * @param       无
 * @retval      无
 */
void slcd_dma_init(void)
{
    RCC->AHBENR |= 1 << 1;              /* 开启DMA2时钟 */
    DMA2_Channel5->CPAR = 0;            /* 暂不设置 */
    DMA2_Channel5->CMAR = (uint32_t)&LCD->LCD_RAM;  /* 目标地址为LCD_RAM */
    DMA2_Channel5->CNDTR = 0;           /* DMA2,传输数据量 */
    DMA2_Channel5->CCR = 0X00000000;    /* 复位 */

    DMA2_Channel5->CCR |= 0 << 4;       /* 从外设读 */
    DMA2_Channel5->CCR |= 0 << 5;       /* 普通模式 */
    DMA2_Channel5->CCR |= 1 << 6;       /* 外设地址增量模式 */
    DMA2_Channel5->CCR |= 0 << 7;       /* 存储器非增量模式 */
    DMA2_Channel5->CCR |= 1 << 8;       /* 外设数据宽度为16位 */
    DMA2_Channel5->CCR |= 1 << 10;      /* 存储器数据宽度16位 */
    DMA2_Channel5->CCR |= 1 << 12;      /* 中等优先级 */
    DMA2_Channel5->CCR |= 1 << 14;      /* 存储器到存储器模式(不需要外部请求) */
}

/**
 * @brief       开启一次SPI到LCD的DMA的传输
 * @param       x       : 起始传输地址编号(0~480)
 * @retval      无
 */
void slcd_dma_enable(uint32_t x)
{
    uint32_t lcdsize = spbdev.spbwidth * spbdev.spbheight;
    uint32_t dmatransfered = 0;

    while (lcdsize)
    {
        DMA2_Channel5->CCR &= ~(1 << 0);    /* 关闭DMA传输 */
        //while(DMA2_Stream0->CR&0X01);     /* 等待DMA2_Stream0可配置 */
        DMA2->IFCR |= 1 << 17;              /* 清除上次的传输完成标记 */

        if (lcdsize > SLCD_DMA_MAX_TRANS)
        {
            lcdsize -= SLCD_DMA_MAX_TRANS;
            DMA2_Channel5->CNDTR = SLCD_DMA_MAX_TRANS;  /* 设置传输长度 */
        }
        else
        {
            DMA2_Channel5->CNDTR = lcdsize;             /* 设置传输长度 */
            lcdsize = 0;
        }

        DMA2_Channel5->CPAR = (uint32_t)(g_sramlcdbuf + x * spbdev.spbheight + dmatransfered);	/* DMA2,改变存储器地址 */
        dmatransfered += SLCD_DMA_MAX_TRANS;
        DMA2_Channel5->CCR |= 1 << 0;           /* 开启DMA传输 */

        while ((DMA2->ISR & (1 << 17)) == 0);   /* 等待 */
    }

    DMA2_Channel5->CCR &= ~(1 << 0);            /* 关闭DMA传输 */
}

/**
 * @brief       显示一帧,即启动一次spi到lcd的显示.
 * @param       x       : 坐标偏移量
 * @retval      无
 */
void slcd_frame_show(uint32_t x)
{
    lcd_scan_dir(U2D_L2R);  /* 设置扫描方向 */

    if (lcddev.id == 0X9341 || lcddev.id == 0X7789 || lcddev.id == 0X5310 || lcddev.id == 0X7796 || lcddev.id == 0X5510 || lcddev.id == 0X9806)
    {
        lcd_set_window(spbdev.stabarheight, 0, spbdev.spbheight, spbdev.spbwidth);
        lcd_set_cursor(spbdev.stabarheight, 0);  /* 设置光标位置 */
    }
    else
    {
        lcd_set_window(0, spbdev.stabarheight, spbdev.spbwidth, spbdev.spbheight);

        if (lcddev.id != 0X1963)lcd_set_cursor(0, spbdev.stabarheight);  /* 设置光标位置 */
    }

    lcd_write_ram_prepare();     /* 开始写入GRAM */
    slcd_dma_enable(x);
    lcd_scan_dir(DFT_SCAN_DIR); /* 恢复默认方向 */
    lcd_set_window(0, 0, lcddev.width, lcddev.height);  /* 恢复默认窗口大小 */
}







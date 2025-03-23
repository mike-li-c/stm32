/**
 ****************************************************************************************************
 * @file        spblcd.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2022-10-26
 * @brief       SPBЧ��ʵ�� ��������
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
 * V1.0 20221026
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#include "./BSP/SPBLCD/spblcd.h"
#include "./BSP/DMA/dma.h"
#include "./MALLOC/malloc.h"
#include "spb.h"


uint16_t *g_sramlcdbuf; /* SRAM LCD BUFFER,����ͼƬ�Դ��� */

/**
 * @brief       ��ָ��λ�û���
 * @param       x,y     : ������
 * @param       color   : ����ɫ
 * @retval      ��
 */
void slcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    g_sramlcdbuf[y + x * spbdev.spbheight] = color;
}

/**
 * @brief       ��ȡָ��λ�õ����ɫֵ
 * @param       x,y     : ������
 * @retval      ��ɫֵ
 */
uint16_t slcd_read_point(uint16_t x, uint16_t y)
{
    return g_sramlcdbuf[y + x * spbdev.spbheight];
}

/**
 * @brief       �����ɫ
 * @param       x,y     : ������
 * @param       width   : ���
 * @param       height  : �߶�
 * @param       *color  : ��ɫ����
 * @retval      ��
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
 * @brief       SRAM��LCD DMA����
 *              16λ,�ⲿSRAM���䵽LCD GRAM
 * @param       ��
 * @retval      ��
 */
void slcd_dma_init(void)
{
    RCC->AHBENR |= 1 << 1;              /* ����DMA2ʱ�� */
    DMA2_Channel5->CPAR = 0;            /* �ݲ����� */
    DMA2_Channel5->CMAR = (uint32_t)&LCD->LCD_RAM;  /* Ŀ���ַΪLCD_RAM */
    DMA2_Channel5->CNDTR = 0;           /* DMA2,���������� */
    DMA2_Channel5->CCR = 0X00000000;    /* ��λ */

    DMA2_Channel5->CCR |= 0 << 4;       /* ������� */
    DMA2_Channel5->CCR |= 0 << 5;       /* ��ͨģʽ */
    DMA2_Channel5->CCR |= 1 << 6;       /* �����ַ����ģʽ */
    DMA2_Channel5->CCR |= 0 << 7;       /* �洢��������ģʽ */
    DMA2_Channel5->CCR |= 1 << 8;       /* �������ݿ��Ϊ16λ */
    DMA2_Channel5->CCR |= 1 << 10;      /* �洢�����ݿ��16λ */
    DMA2_Channel5->CCR |= 1 << 12;      /* �е����ȼ� */
    DMA2_Channel5->CCR |= 1 << 14;      /* �洢�����洢��ģʽ(����Ҫ�ⲿ����) */
}

/**
 * @brief       ����һ��SPI��LCD��DMA�Ĵ���
 * @param       x       : ��ʼ�����ַ���(0~480)
 * @retval      ��
 */
void slcd_dma_enable(uint32_t x)
{
    uint32_t lcdsize = spbdev.spbwidth * spbdev.spbheight;
    uint32_t dmatransfered = 0;

    while (lcdsize)
    {
        DMA2_Channel5->CCR &= ~(1 << 0);    /* �ر�DMA���� */
        //while(DMA2_Stream0->CR&0X01);     /* �ȴ�DMA2_Stream0������ */
        DMA2->IFCR |= 1 << 17;              /* ����ϴεĴ�����ɱ�� */

        if (lcdsize > SLCD_DMA_MAX_TRANS)
        {
            lcdsize -= SLCD_DMA_MAX_TRANS;
            DMA2_Channel5->CNDTR = SLCD_DMA_MAX_TRANS;  /* ���ô��䳤�� */
        }
        else
        {
            DMA2_Channel5->CNDTR = lcdsize;             /* ���ô��䳤�� */
            lcdsize = 0;
        }

        DMA2_Channel5->CPAR = (uint32_t)(g_sramlcdbuf + x * spbdev.spbheight + dmatransfered);	/* DMA2,�ı�洢����ַ */
        dmatransfered += SLCD_DMA_MAX_TRANS;
        DMA2_Channel5->CCR |= 1 << 0;           /* ����DMA���� */

        while ((DMA2->ISR & (1 << 17)) == 0);   /* �ȴ� */
    }

    DMA2_Channel5->CCR &= ~(1 << 0);            /* �ر�DMA���� */
}

/**
 * @brief       ��ʾһ֡,������һ��spi��lcd����ʾ.
 * @param       x       : ����ƫ����
 * @retval      ��
 */
void slcd_frame_show(uint32_t x)
{
    lcd_scan_dir(U2D_L2R);  /* ����ɨ�跽�� */

    if (lcddev.id == 0X9341 || lcddev.id == 0X7789 || lcddev.id == 0X5310 || lcddev.id == 0X7796 || lcddev.id == 0X5510 || lcddev.id == 0X9806)
    {
        lcd_set_window(spbdev.stabarheight, 0, spbdev.spbheight, spbdev.spbwidth);
        lcd_set_cursor(spbdev.stabarheight, 0);  /* ���ù��λ�� */
    }
    else
    {
        lcd_set_window(0, spbdev.stabarheight, spbdev.spbwidth, spbdev.spbheight);

        if (lcddev.id != 0X1963)lcd_set_cursor(0, spbdev.stabarheight);  /* ���ù��λ�� */
    }

    lcd_write_ram_prepare();     /* ��ʼд��GRAM */
    slcd_dma_enable(x);
    lcd_scan_dir(DFT_SCAN_DIR); /* �ָ�Ĭ�Ϸ��� */
    lcd_set_window(0, 0, lcddev.width, lcddev.height);  /* �ָ�Ĭ�ϴ��ڴ�С */
}







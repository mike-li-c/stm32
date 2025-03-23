/**
 ****************************************************************************************************
 * @file        main.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-29
 * @brief       ���ֲ����� ʵ��
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

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./USMART/usmart.h"
#include "./MALLOC/malloc.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./TEXT/text.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SRAM/sram.h"
#include "./BSP/SDIO/sdio_sdcard.h"
#include "./BSP/NORFLASH/norflash.h"
#include "./BSP/VS10XX/vs10xx.h"
#include "./APP/audioplayer.h"


int main(void)
{
    sys_stm32_clock_init(9);    /* ����ʱ��, 72Mhz */
    delay_init(72);             /* ��ʱ��ʼ�� */
    usart_init(72, 115200);     /* ���ڳ�ʼ��Ϊ115200 */
    usmart_dev.init(72);        /* ��ʼ��USMART */
    led_init();                 /* ��ʼ��LED */
    lcd_init();                 /* ��ʼ��LCD */
    key_init();                 /* ��ʼ������ */
    sram_init();                /* SRAM��ʼ�� */
    norflash_init();            /* ��ʼ��NORFLASH */
    vs10xx_init();              /* VS10XX��ʼ�� */
    my_mem_init(SRAMIN);        /* ��ʼ���ڲ�SRAM�ڴ�� */
    my_mem_init(SRAMEX);        /* ��ʼ���ⲿSRAM�ڴ�� */

    exfuns_init();              /* Ϊfatfs��ر��������ڴ� */
    f_mount(fs[0], "0:", 1);    /* ����SD�� */
    f_mount(fs[1], "1:", 1);    /* ����FLASH */

    while (fonts_init())        /* ����ֿ� */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        delay_ms(200);
        lcd_fill(30, 50, 240, 66, WHITE);   /* �����ʾ */
        delay_ms(200);
    }

    text_show_string(30, 50, 200, 16, "����ԭ��STM32������", 16, 0, RED);
    text_show_string(30, 70, 200, 16, "���ֲ����� ʵ��", 16, 0, RED);
    text_show_string(30, 110, 200, 16, "KEY0:NEXT   KEY2:PREV", 16, 0, RED);
    text_show_string(30, 130, 200, 16, "KEY_UP:VOL+ KEY1:VOL-", 16, 0, RED);

    while (1)
    {
        LED1(0);
        text_show_string(30, 150, 200, 16, "�洢������...", 16, 0, RED);
        printf("Ram Test:0X%04X\r\n", vs10xx_ram_test());   /* ��ӡRAM���Խ�� */
        
        text_show_string(30, 150, 200, 16, "���Ҳ�����...", 16, 0, RED);
        vs10xx_sine_test();

        text_show_string(30, 150, 200, 16, "<<���ֲ�����>>", 16, 0, RED);
        LED1(1);
        audio_play();
    }
}











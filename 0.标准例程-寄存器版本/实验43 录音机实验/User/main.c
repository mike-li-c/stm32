 /**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-05-10
 * @brief       录音机 实验
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
#include "./BSP/TPAD/tpad.h"
#include "./APP/recorder.h"


int main(void)
{
    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    usmart_dev.init(72);        /* 初始化USMART */
    led_init();                 /* 初始化LED */
    lcd_init();                 /* 初始化LCD */
    key_init();                 /* 初始化按键 */
    sram_init();                /* SRAM初始化 */
    norflash_init();            /* 初始化NORFLASH */
    tpad_init(6);               /* 初始化TPAD */
    vs10xx_init();              /* VS10XX初始化 */
    my_mem_init(SRAMIN);        /* 初始化内部SRAM内存池 */
    my_mem_init(SRAMEX);        /* 初始化外部SRAM内存池 */

    exfuns_init();              /* 为fatfs相关变量申请内存 */
    f_mount(fs[0], "0:", 1);    /* 挂载SD卡 */
    f_mount(fs[1], "1:", 1);    /* 挂载FLASH */

    while (fonts_init())        /* 检查字库 */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        delay_ms(200);
        lcd_fill(30, 50, 240, 66, WHITE);   /* 清除显示 */
        delay_ms(200);
    }

    text_show_string(30,  50, 200, 16, "正点原子STM32开发板", 16, 0, RED);
    text_show_string(30,  70, 200, 16, "WAV录音机 实验", 16, 0, RED);
    text_show_string(30, 110, 200, 16, "KEY0:REC/PAUSE", 16, 0, RED);
    text_show_string(30, 130, 200, 16, "KEY2:STOP&SAVE", 16, 0, RED);
    text_show_string(30, 150, 200, 16, "KEY_UP:AGC+ KEY1:AGC-", 16, 0, RED);
    text_show_string(30, 170, 200, 16, "TPAD:Play The File", 16, 0, RED);

    while (1)
    {
        LED1(0);
        text_show_string(30, 190, 200, 16, "存储器测试...", 16, 0, RED);
        printf("Ram Test:0X%04X\r\n", vs10xx_ram_test());   /* 打印RAM测试结果 */
        
        text_show_string(30, 190, 200, 16, "正弦波测试...", 16, 0, RED);
        vs10xx_sine_test();

        text_show_string(30, 190, 200, 16, "<<WAV录音机>>", 16, 0, RED);
        LED1(1);
        recoder_play();
    }
}











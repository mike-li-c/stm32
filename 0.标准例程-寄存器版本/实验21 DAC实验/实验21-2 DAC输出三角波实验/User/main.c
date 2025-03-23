/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-24
 * @brief       DAC输出三角波 实验
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
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/DAC/dac.h"


int main(void)
{
    uint8_t t = 0; 
    uint8_t key;

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    usmart_dev.init(240);       /* 初始化USMART */
    led_init();                 /* 初始化LED */
    lcd_init();                 /* 初始化LCD */
    key_init();                 /* 初始化按键 */
    dac_init(1);                /* 初始化DAC1_OUT1通道 */

    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "DAC Triangular WAVE TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEY0:Wave1  KEY1:Wave2", RED);
    lcd_show_string(30, 130, 200, 16, 16, "DAC None", BLUE); /* 提示无输出 */ 

    while (1)
    {
        t++;
        key = key_scan(0);  /* 按键扫描 */

        if (key == KEY0_PRES)       /* 高采样率 , 约1Khz波形 */
        {
            lcd_show_string(30, 130, 200, 16, 16, "DAC Wave1 ", BLUE); 
            dac_triangular_wave(4095, 5, 2000, 100);    /* 幅值4095, 采样点间隔5us, 200个采样点, 100个波形 */
            lcd_show_string(30, 130, 200, 16, 16, "DAC None  ", BLUE); 
       }
        else if (key == KEY1_PRES)  /* 低采样率 , 约1Khz波形 */
        {
            lcd_show_string(30, 130, 200, 16, 16, "DAC Wave2 ", BLUE); 
            dac_triangular_wave(4095, 500, 20, 100);    /* 幅值4095, 采样点间隔500us, 20个采样点, 100个波形 */ 
            lcd_show_string(30, 130, 200, 16, 16, "DAC None  ", BLUE); 
        }

        if (t == 10 )       /* 定时时间到了 */
        { 
            LED0_TOGGLE();  /* LED0闪烁 */
            t = 0;
        }

        delay_ms(10);
    }
}































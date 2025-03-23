/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-26
 * @brief       游戏手柄 实验
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
#include "./BSP/JOYPAD/joypad.h"


/* 手柄按键符号定义 */
const char *JOYPAD_SYMBOL_TBL[8] ={"Right", "Left", "Down", "Up", "Start", "Select", "B", "A"};

int main(void)
{
    uint8_t key;
    uint8_t t = 0, i = 0;

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    led_init();                 /* 初始化LED */
    lcd_init();                 /* 初始化LCD */
    joypad_init();              /* 游戏手柄接口初始化 */

    lcd_show_string(30,  50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30,  70, 200, 16, 16, "JOYPAD TEST", RED);
    lcd_show_string(30,  90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEYVAL:", RED);
    lcd_show_string(30, 130, 200, 16, 16, "SYMBOL:", RED);

    while (1)
    {
        key = joypad_read();

        if (key)    /* 手柄 有按键按下 */
        {
            lcd_show_num(116, 110, key, 3, 16, BLUE); /* 显示键值 */

            for (i = 0; i < 8; i++)
            {
                if (key & (0X80 >> i))
                {
                    lcd_fill(30 + 56, 130, 30 + 56 + 48, 150 + 16, WHITE); /* 清除之前的显示 */
                    lcd_show_string(30 + 56, 130, 200, 16, 16, (char *)JOYPAD_SYMBOL_TBL[i], BLUE); /* 显示符号 */
                }
            }
        }

        delay_ms(10);
        t++;

        if (t == 20)
        {
            t = 0;
            LED0_TOGGLE();  /* LED0闪烁 */
        }
    }
}













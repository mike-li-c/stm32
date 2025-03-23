/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-27
 * @brief       外部SRAM 实验
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
#include "./BSP/SRAM/sram.h"


/* 测试用数组, 起始地址为: SRAM_BASE_ADDR */
#if (__ARMCC_VERSION >= 6010050)
uint32_t g_test_buffer[250000] __attribute__((section(".bss.ARM.__at_0x68000000")));
#else
uint32_t g_test_buffer[250000] __attribute__((at(SRAM_BASE_ADDR)));
#endif

/**
 * @brief       外部内存测试(最大支持1M字节内存测试)
 * @param       无
 * @retval      无
 */
void fsmc_sram_test(uint16_t x, uint16_t y)
{
    uint32_t i = 0;
    uint8_t temp = 0;
    uint8_t sval = 0;   /* 在地址0读到的数据 */

    lcd_show_string(x, y, 239, y + 16, 16, "Ex Memory Test:   0KB", BLUE);

    /* 每隔4K字节,写入一个数据,总共写入256个数据,刚好是1M字节 */
    for (i = 0; i < 1024 * 1024; i += 4096)
    {
        sram_write(&temp, i, 1);
        temp++;
    }

    /* 依次读出之前写入的数据,进行校验 */
    for (i = 0; i < 1024 * 1024; i += 4096)
    {
        sram_read(&temp, i, 1);

        if (i == 0)
        {
            sval = temp;
        }
        else if (temp <= sval)
        {
            break;  /* 后面读出的数据一定要比第一次读到的数据大 */
        }
        
        lcd_show_xnum(x + 15 * 8, y, (uint16_t)(temp - sval + 1) * 4, 4, 16, 0, BLUE); /* 显示内存容量 */
    }
}

int main(void)
{
    uint8_t key;
    uint8_t i = 0;
    uint32_t ts = 0;

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    usmart_dev.init(72);        /* 初始化USMART */
    led_init();                 /* 初始化LED */
    lcd_init();                 /* 初始化LCD */
    key_init();                 /* 初始化按键 */
    sram_init();                /* SRAM初始化 */

    lcd_show_string(30,  50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30,  70, 200, 16, 16, "SRAM TEST", RED);
    lcd_show_string(30,  90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEY0:Test Sram", RED);
    lcd_show_string(30, 130, 200, 16, 16, "KEY1:TEST Data", RED);
    

    for (ts = 0; ts < 250000; ts++)
    {
        g_test_buffer[ts] = ts;         /* 预存测试数据 */
    }
    
    while (1)
    {
        key = key_scan(0);              /* 不支持连按 */

        if (key == KEY0_PRES)
        {
            fsmc_sram_test(30, 150);    /* 测试SRAM容量 */
        }
        else if (key == KEY1_PRES)      /* 打印预存测试数据 */
        {
            for (ts = 0; ts < 250000; ts++)
            {
                lcd_show_xnum(30, 170, g_test_buffer[ts], 6, 16, 0, BLUE);  /* 显示测试数据 */
            }
        }
        else
        {
            delay_ms(10);
        }
        
        i++;

        if (i == 20)
        {
            i = 0;
            LED0_TOGGLE();  /* LED0闪烁 */
        }
    }
}











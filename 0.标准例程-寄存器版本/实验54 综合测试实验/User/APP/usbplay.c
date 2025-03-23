/**
 ****************************************************************************************************
 * @file        usbplay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-10-31
 * @brief       APP-USB连接 代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.1 20221031
 * 1, 修改注释方式
 * 2, 修改u8/u16/u32为uint8_t/uint16_t/uint32_t
 ****************************************************************************************************
 */

#include "usbplay.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage.h"
#include "./BSP/SDIO/sdio_sdcard.h"



USBD_HandleTypeDef USBD_Device;             /* USB Device处理结构体 */

extern volatile uint8_t g_usb_state_reg;    /* USB状态 */ 

/**
 * @brief       USB连接
 * @param       无
 * @retval      0, 正常退出; 其他, 错误代码;
 */
uint8_t usb_play(void)
{
    uint8_t rval = 0;           //返回值
    uint8_t offline_cnt = 0;
    uint8_t tct = 0;
    
    uint8_t USB_STA = 0;
    uint8_t busycnt = 0;    /* USB忙计数器 */
    uint8_t errcnt = 0;     /* USB错误计数器 */
    
    
    g_back_color = LGRAY;
    lcd_clear(g_back_color);

    app_filebrower((uint8_t *)APP_MFUNS_CAPTION_TBL[12][gui_phy.language], 0X05); //显示标题
    
    lcd_show_string(60 + (lcddev.width - 240) / 2, 130 + (lcddev.height - 320) / 2, lcddev.width, lcddev.height, 16, "USB DisConnected ", BLUE); /* 首先提示USB无连接 */

    usbd_port_config(0);    /* USB先断开 */
    delay_ms(500);
    usbd_port_config(1);    /* USB再次连接 */
    delay_ms(500);

    USBD_Init(&USBD_Device, &MSC_Desc, 0);                      /* 初始化USB */
    USBD_RegisterClass(&USBD_Device, USBD_MSC_CLASS);           /* 添加类 */
    USBD_MSC_RegisterStorage(&USBD_Device, &USBD_DISK_fops);    /* 为MSC类添加回调函数 */
    USBD_Start(&USBD_Device);                                   /* 开启USB */

    while (1)
    {
        delay_ms(1000 / OS_TICKS_PER_SEC);  /* 延时一个时钟节拍 */

        if (system_task_return)
        {
            if (busycnt) /* USB正在读写 */
            {
                lcd_show_string(60 + (lcddev.width - 240) / 2, 110 + (lcddev.height - 320) / 2, lcddev.width, lcddev.height, 16, "USB BUSY!!!", RED); /* 提示USB正忙 */
                system_task_return = 0;     /* 取消 */
            }
            else break; /* USB空闲,则退出USB */
        }

        tct++;

        if (tct == 40) /* 每40ms进入一次 */
        {
            tct = 0;

            if (busycnt)busycnt--;
            else gui_fill_rectangle(60 + (lcddev.width - 240) / 2, 110 + (lcddev.height - 320) / 2, 100, 16, g_back_color); /* 清除显示 */

            if (errcnt)errcnt--;
            else gui_fill_rectangle(60 + (lcddev.width - 240) / 2, 170 + (lcddev.height - 320) / 2, 128, 16, g_back_color); /* 清除显示 */

            if (g_usb_state_reg & 0x10)         /* 有轮询操作 */
            {
                offline_cnt = 0;                /* USB连接了,则清除offline计数器 */
                g_usb_state_reg |= 0X80;        /* 标记USB连接正常 */
                g_usb_state_reg &= ~(1 << 4);   /* 清除轮询标志位 */
            }
            else
            {
                offline_cnt++;

                if (offline_cnt > 50)g_usb_state_reg = 0; /* 2s内没收到在线标记,代表USB被拔出了 */
            }
        }

        if (USB_STA != g_usb_state_reg) /* 状态改变了 */
        {
            gui_fill_rectangle(60 + (lcddev.width - 240) / 2, 150 + (lcddev.height - 320) / 2, 120, 16, g_back_color); /* 清除显示 */

            if (g_usb_state_reg & 0x01) /* 正在写 */
            {
                if (busycnt < 5)busycnt++;

                lcd_show_string(60 + (lcddev.width - 240) / 2, 150 + (lcddev.height - 320) / 2, lcddev.width, lcddev.height, 16, "USB Writing...", BLUE); /* 提示USB正在写入数据 */
            }

            if (g_usb_state_reg & 0x02) /* 正在读 */
            {
                if (busycnt < 5)busycnt++;

                lcd_show_string(60 + (lcddev.width - 240) / 2, 150 + (lcddev.height - 320) / 2, lcddev.width, lcddev.height, 16, "USB Reading...", BLUE); /* 提示USB正在读出数据 */
            }

            if (g_usb_state_reg & 0x04)
            {
                if (errcnt < 5)errcnt++;

                lcd_show_string(60 + (lcddev.width - 240) / 2, 170 + (lcddev.height - 320) / 2, lcddev.width, lcddev.height, 16, "USB Write Error", RED); /* 提示写入错误 */
            }

            if (g_usb_state_reg & 0x08)
            {
                if (errcnt < 5)errcnt++;

                lcd_show_string(60 + (lcddev.width - 240) / 2, 170 + (lcddev.height - 320) / 2, lcddev.width, lcddev.height, 16, "USB Read  Error", RED); /* 提示读出错误 */
            }
            
            if (g_usb_state_reg & 0X80)lcd_show_string(60 + (lcddev.width - 240) / 2, 130 + (lcddev.height - 320) / 2, lcddev.width, lcddev.height, 16, "USB Connected   ", BLUE);    /* 提示USB连接已经建立 */
            else lcd_show_string(60 + (lcddev.width - 240) / 2, 130 + (lcddev.height - 320) / 2, lcddev.width, 320, 16, "USB DisConnected", BLUE);  /* 提示USB被拔出了 */

            g_usb_state_reg &= 0X90;      /* 清除除连接状态&轮询标志外的其他所有位 */
            USB_STA = g_usb_state_reg;    /* 记录最后的状态 */
        }
    }
    
    usbd_port_config(0);            /* 关闭 USB */
    
    USBD_Stop(&USBD_Device);        /* 停止USB */
    
    memset(&USBD_Device, 0, sizeof(USBD_HandleTypeDef));
    
    return rval;
}








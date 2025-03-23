/**
 ****************************************************************************************************
 * @file        camera.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-10-31
 * @brief       APP-照相机 代码
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

#include "camera.h"
#include "calendar.h"
#include "common.h"
#include "./BSP/OV7725/ov7725.h"
#include "./BSP/EXTI/exti.h"


extern volatile uint8_t framecnt;       /* 在timer.c里 面定义 */
extern volatile uint8_t g_ov7725_vsta;  /* 在exit.c里 面定义 */

/* 
 * 受限于OV7725模组的FIFO的容量，无法存下一帧640*480的画像，我们这里QVGA和VGA模式都采用
 * 320*240的分辨率,VGA模式下支持缩放，所以可以使用更小的分辨率来显示，读者可以根据需要自行修改
 */
uint16_t g_ov7725_wwidth  = 320;    /* 默认窗口宽度为320 */
uint16_t g_ov7725_wheight = 240;    /* 默认窗口高度为240 */


/* 摄像头提示 */
uint8_t *const camera_remind_tbl[3][GUI_LANGUAGE_NUM] =
{
    {"初始化OV7725,请稍侯...", "初始化OV7725,請稍後...", "OV7725 Init,Please wait...",},
    {"未检测到OV7725,请检查...", "未檢測到OV7725,請檢查...", "No OV7725 find,Please check...",},
    {"保存为:", "保存為:", "SAVE AS:",},
};
/* 拍照成功提示框标题 */
uint8_t *const camera_saveok_caption[GUI_LANGUAGE_NUM] =
{
    "拍照成功！", "拍照成功！", "Take Photo OK！",
};

/* 00级功能选项表标题 */
uint8_t *const camera_l00fun_caption[GUI_LANGUAGE_NUM] =
{
    "相机设置", "相機設置", "Camera Set",
};

/* 10级功能选项表 */
uint8_t *const camera_l00fun_table[GUI_LANGUAGE_NUM][5] =
{
    {"场景设置", "特效设置", "亮度设置", "色度设置", "对比度设置",},
    {"場景設置", "特效設置", "亮度設置", "色度設置", "對比度設置",},
    {"Scene", "Effects", "Brightness", "Saturation", "Contrast"},
};

/* 11级功能选项表, 灯光模式 */
uint8_t *const camera_l11fun_table[GUI_LANGUAGE_NUM][6] =
{
    {"自动", "晴天", "阴天", "办公室", "家庭", "夜晚",},
    {"自動", "晴天", "陰天", "辦公室", "家庭", "夜晚",},
    {"Auto", "Sunny", "Cloudy", "Office", "Home", "Night"},
};

/* 12级功能选项表, 特效设置 */
uint8_t *const camera_l12fun_table[GUI_LANGUAGE_NUM][7] =
{
    {"普通", "负片", "黑白", "偏红色", "偏绿色", "偏蓝色", "复古"},
    {"普通", "負片", "黑白", "偏紅色", "偏綠色", "偏藍色", "復古"},
    {"Normal", "Negative", "B&W", "Redish", "Greenish", "Bluish", "Antique",},
};

/* 13~15级功能选项表, 都是-3~3的7個值 */
uint8_t *const camera_l135fun_table[GUI_LANGUAGE_NUM][7] =
{
    {"-3", "-2", "-1", "0", "+1", "+2", "+3"},
    {"-3", "-2", "-1", "0", "+1", "+2", "+3"},
    {"-3", "-2", "-1", "0", "+1", "+2", "+3"},
};

/**
 * @brief       更新LCD显示
 * @param       无
 * @retval      无
 */
void camera_refresh(void)
{
    uint32_t i, j;
    uint16_t color;
 
    if (g_ov7725_vsta)                  /* 有帧中断更新 */
    {
        lcd_scan_dir(U2D_L2R);          /* 从上到下, 从左到右 */
        lcd_set_window((lcddev.width - g_ov7725_wwidth) / 2, (lcddev.height - g_ov7725_wheight) / 2,
                        g_ov7725_wwidth, g_ov7725_wheight);     /* 将显示区域设置到屏幕中央 */

        lcd_write_ram_prepare();        /* 开始写入GRAM */

        OV7725_RRST(0);                 /* 开始复位读指针 */
        OV7725_RCLK(0);
        OV7725_RCLK(1);
        OV7725_RCLK(0);
        OV7725_RRST(1);                 /* 复位读指针结束 */
        OV7725_RCLK(1);

        for (i = 0; i < g_ov7725_wheight; i++)
        {
            for (j = 0; j < g_ov7725_wwidth; j++)
            {
                OV7725_RCLK(0);
                color = OV7725_DATA;    /* 读数据 */
                OV7725_RCLK(1);
                color <<= 8;
                OV7725_RCLK(0);
                color |= OV7725_DATA;   /* 读数据 */
                OV7725_RCLK(1);
                LCD->LCD_RAM = color;
            }
        }

        g_ov7725_vsta = 0;              /* 清零帧中断标记 */
        framecnt++;
        lcd_scan_dir(DFT_SCAN_DIR);     /* 恢复默认扫描方向 */
    }
}

/**
 * @brief       通过时间获取文件名
 *  @note       组合成:形如"0:PHOTO/PIC20120321210633.bmp"/"2:PHOTO/PIC20120321210633.bmp"的文件名
 * @param       pname           : 带路径的名字
 * @retval      无
 */
void camera_new_pathname(uint8_t *pname)
{
    calendar_get_time(&calendar);
    calendar_get_date(&calendar);

    if (gui_phy.memdevflag & (1 << 0))sprintf((char *)pname, "0:PHOTO/PIC%04d%02d%02d%02d%02d%02d.bmp", calendar.year, calendar.month, calendar.date, calendar.hour, calendar.min, calendar.sec);       /* 首选保存在SD卡 */
    else if (gui_phy.memdevflag & (1 << 2))sprintf((char *)pname, "2:PHOTO/PIC%04d%02d%02d%02d%02d%02d.bmp", calendar.year, calendar.month, calendar.date, calendar.hour, calendar.min, calendar.sec);  /* SD卡不存在,则保存在U盘 */
}

/**
 * @brief       照相机 功能
 *  @note       所有照片文件,均保存在SD卡PHOTO文件夹内.
 * @param       无
 * @retval      0, 正常退出; 其他, 错误代码;
 */
uint8_t camera_play(void)
{
    uint8_t rval = 0;
    uint8_t res;
    uint8_t *caption = 0;
    uint8_t *pname;
    uint8_t selx = 0;
    uint8_t l00sel = 0, l11sel = 0, l12sel = 0; /* 默认选择项 */
    uint8_t l345sel[3];
    uint8_t *psn;

    /* 提示开始检测OV7725 */
    window_msg_box((lcddev.width - 200) / 2, (lcddev.height - 80) / 2, 200, 80, (uint8_t *)camera_remind_tbl[0][gui_phy.language], (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language], 12, 0, 0, 0);

    if (gui_phy.memdevflag & (1 << 0))f_mkdir("0:PHOTO"); /* 强制创建文件夹,给照相机用 */

    if (ov7725_init())  /* 初始化OV7725 */
    {
        window_msg_box((lcddev.width - 200) / 2, (lcddev.height - 80) / 2, 200, 80, (uint8_t *)camera_remind_tbl[1][gui_phy.language], (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language], 12, 0, 0, 0);
        rval = 1;
    }
    
    delay_ms(1500);

    pname = gui_memin_malloc(40);   /* 申请40个字节内存,用于存放文件名 */
    psn = gui_memin_malloc(50);     /* 申请50个字节内存,用于存放类似：“保存为:0:PHOTO/PIC20120321210633.bmp"”的提示语 */

    if (pname == NULL)rval = 1;     /* 申请失败 */

    if (rval == 0)  /* OV7725正常\申请内存成功 */
    {
        l345sel[0] = 2;             /* 亮度默认为2,实际值0 */
        l345sel[1] = 2;             /* 色度默认为2,实际值0 */
        l345sel[2] = 2;             /* 对比度默认为2,实际值0 */

        ov7725_window_set(g_ov7725_wwidth, g_ov7725_wheight, 0);    /* QVGA模式输出 */

        OV7725_OE(0);               /* 使能OV7725 FIFO数据输出 */

        lcd_clear(BLACK);
        exti_ov7725_vsync_init();   /* 使能OV7725 VSYNC外部中断, 捕获帧中断 */
        
        tim3_int_init(10000 - 1, 7200 - 1); /* 启动TIM3 ,1s中断一次 */

        while (1)
        {
            tp_dev.scan(0);

            if (tp_dev.sta & TP_PRES_DOWN)
            {
                caption = (uint8_t *)camera_l00fun_caption[gui_phy.language];
                res = app_items_sel((lcddev.width - 160) / 2, (lcddev.height - 72 - 32 * 5) / 2, 160, 72 + 32 * 5, (uint8_t **)camera_l00fun_table[gui_phy.language], 5, (uint8_t *)&l00sel, 0X90, caption); /* 单选 */

                if (res == 0)
                {
                    caption = (uint8_t *)camera_l00fun_table[gui_phy.language][l00sel];
                    camera_refresh();   /* 刷新一下 */

                    switch (l00sel)
                    {
                        case 0:/* 灯光模式设置 */
                            res = app_items_sel((lcddev.width - 160) / 2, (lcddev.height - 72 - 32 * 6) / 2, 160, 72 + 32 * 6, (uint8_t **)camera_l11fun_table[gui_phy.language], 6, (uint8_t *)&l11sel, 0X90, caption); /* 单选 */

                            if (res == 0)
                            {
                                ov7725_light_mode(l11sel);  /* 灯光模式设置 */
                            }

                            break;

                        case 1:/* 特效设置 */
                            res = app_items_sel((lcddev.width - 160) / 2, (lcddev.height - 72 - 32 * 7) / 2, 160, 72 + 32 * 7, (uint8_t **)camera_l12fun_table[gui_phy.language], 7, (uint8_t *)&l12sel, 0X90, caption); /* 单选 */

                            if (res == 0)
                            {
                                ov7725_special_effects(l12sel); /* 特效设置 */
                            }

                            break;

                        case 2:/* 亮度,色度,对比度设置 */
                        case 3:/* 亮度,色度,对比度设置 */
                        case 4:/* 亮度,色度,对比度设置 */
                            selx = l345sel[l00sel - 2]; /* 得到之前的选择 */
                            res = app_items_sel((lcddev.width - 160) / 2, (lcddev.height - 72 - 32 * 7) / 2, 160, 72 + 32 * 7, (uint8_t **)camera_l135fun_table[gui_phy.language], 7, (uint8_t *)&selx, 0X90, caption); /* 单选 */

                            if (res == 0)
                            {
                                l345sel[l00sel - 2] = selx; /* 记录新值 */

                                if (l00sel == 2)ov7725_brightness(selx);        /* 亮度设置 */

                                if (l00sel == 3)ov7725_color_saturation(selx);  /* 色度设置 */

                                if (l00sel == 4)ov7725_contrast(selx);          /* 对比度设置 */
                            }

                            break;
                    }
                }

                lcd_clear(BLACK);
            }

            if (system_task_return)break; /* TPAD返回 */

            camera_refresh();

            if (key_scan(0) == WKUP_PRES) /* KEY_UP按下了一次,拍照一次 */
            {
                LED1(0);            /* DS1亮,提示正在保存 */
                pname[0] = '\0';    /* 添加结束符 */
                
                camera_new_pathname(pname);

                if (pname != NULL)
                {
                    if (bmp_encode(pname, (lcddev.width - 240) / 2, (lcddev.height - 320) / 2, 240, 320, 0)) /* 拍照有误 */
                    {
                        window_msg_box((lcddev.width - 200) / 2, (lcddev.height - 100) / 2, 200, 100, (uint8_t *)APP_CREAT_ERR_MSG_TBL[gui_phy.language], (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language], 12, 0, 0, 0); /* 提示SD卡是否存在 */
                        delay_ms(2000);/* 等待2秒钟 */
                    }
                    else
                    {
                        strcpy((char *)psn, (const char *)camera_remind_tbl[2][gui_phy.language]);
                        strcat((char *)psn, (const char *)pname);
                        window_msg_box((lcddev.width - 180) / 2, (lcddev.height - 80) / 2, 180, 80, psn, (uint8_t *)camera_saveok_caption[gui_phy.language], 12, 0, 0, 0);
                        BEEP(1);            /* 蜂鸣器短叫，提示拍照完成 */
                        delay_ms(100);
                    }

                    system_task_return = 0; /* 清除TPAD */
                }

                LED1(1);    /* DS1灭,提示保存完成 */
                BEEP(0);    /* 蜂鸣器短叫 */
                delay_ms(1800);
            }
        }
    }

    gui_memin_free(pname);
    gui_memin_free(psn);
    
    TIM3->CR1 &= ~(1 << 0); /* 关闭定时器3 */
    EXTI->IMR &= ~(1 << 8); /* 关闭line8中断 */
    
    return 0;
}
















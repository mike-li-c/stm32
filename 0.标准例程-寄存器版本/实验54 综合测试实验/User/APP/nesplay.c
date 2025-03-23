/**
 ****************************************************************************************************
 * @file        nesplay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.2
 * @date        2022-10-28
 * @brief       APP-NES模拟器 代码
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
 * V1.1 20160627
 * 新增对SMS游戏的支持
 * V1.2 20221028
 * 1, 修改注释方式
 * 2, 修改uint8_t/uint16_t/uint32_t为uint8_t/uint16_t/uint32_t
 ****************************************************************************************************
 */

#include "nesplay.h"
#include "nes_main.h"
#include "spb.h"
#include "audioplay.h"
#include "./BSP/USART3/usart3.h"


/* ROM指针 */
uint8_t *rom_file;

/* 标题 */
uint8_t *const nes_caption_tbl[GUI_LANGUAGE_NUM] = {"NES模拟器", "NES模擬器", "NES Emulator",};

/* 手柄接入提示 */
uint8_t *const nes_remindmsg_tbl[GUI_LANGUAGE_NUM] =
{
    "1,P8:PB10接COM3_RX,PB11接COM3_TX\r\
2,K1:打到JOYPAD档\r\
3,将手柄插入COM3接口\r\
4,退出本应用后，请恢复原来的设置！",
    "1,P8:PB10接COM3_RX,PB11接COM3_TX\r\
2,K1:打到JOYPAD檔\r\
3,將手柄插入COM3接口\r\
4,退出本應用後，請恢復原來的設置！",
    "1,P8:PB10-COM3_RX,PB11-COM3_TX\r\
2,K1:Turn to JOYPAD\r\
3,Insert GAMEPAD to COM3\r\
4,Exit,restore the orig settings",
};

/* 错误提示 */
uint8_t *const nes_errormsg_tbl[3][GUI_LANGUAGE_NUM] =
{
    {"内存不够!", "內存不夠!", "Out of memory!",},
    {"文件读取错误!", "文件讀取錯誤!", "Read file error!",},
    {"MAP不支持!", "MAP不支持!", "Not supported MAP!",},
};

/**
 * @brief       加载游戏界面
 * @param       无
 * @retval      无
 */
void nes_load_ui(void)
{
    app_filebrower((uint8_t *)nes_caption_tbl[gui_phy.language], 0X05);     /* 显示标题 */
    gui_fill_rectangle(0, 20, lcddev.width, lcddev.height - 20, BLACK);     /* 填充底色 */
}

/**
 * @brief       开始游戏
 * @param       无
 * @retval      无
 */
uint8_t nes_play(void)
{
    DIR nesdir;         /* nesdir专用 */
    FILINFO *nesinfo;
    uint8_t res;
    uint8_t rval = 0;   /* 返回值 */
    uint8_t *pname = 0;
    uint8_t *fn;

    _btn_obj *rbtn;     /* 返回按钮控件 */
    _filelistbox_obj *flistbox;
    _filelistbox_list *filelistx;   /* 文件 */

    app_muti_remind_msg((lcddev.width - 210) / 2, (lcddev.height - 158) / 2, 210, 158, APP_REMIND_CAPTION_TBL[gui_phy.language], nes_remindmsg_tbl[gui_phy.language]);
    lcd_clear(BLACK);
    app_filebrower((uint8_t *)APP_MFUNS_CAPTION_TBL[6][gui_phy.language], 0X07);    /* 选择目标文件,并得到目标数量 */
    flistbox = filelistbox_creat(0, gui_phy.tbheight, lcddev.width, lcddev.height - gui_phy.tbheight * 2, 1, gui_phy.listfsize); /* 创建一个filelistbox */

    if (flistbox == NULL)rval = 1;          /* 申请内存失败 */
    else
    {
        flistbox->fliter = FLBOX_FLT_NES;   /* 查找文本文件 */
        filelistbox_add_disk(flistbox);     /* 添加磁盘路径 */
        filelistbox_draw_listbox(flistbox);
    }

    /* 为长文件名申请缓存区 */
    
    nesinfo = gui_memin_malloc(sizeof(FILINFO));    /* 为文件信息申请内存 */
 

    rbtn = btn_creat(lcddev.width - 2 * gui_phy.tbfsize - 8 - 1, lcddev.height - gui_phy.tbheight, 2 * gui_phy.tbfsize + 8, gui_phy.tbheight - 1, 0, 0x03); /* 创建文字按钮 */

    if (rbtn == NULL)rval = 1;  /* 没有足够内存够分配 */
    else
    {
        rbtn->caption = (uint8_t *)GUI_BACK_CAPTION_TBL[gui_phy.language]; /* 返回 */
        rbtn->font = gui_phy.tbfsize;   /* 设置新的字体大小 */
        rbtn->bcfdcolor = WHITE;        /* 按下时的颜色 */
        rbtn->bcfucolor = WHITE;        /* 松开时的颜色 */
        btn_draw(rbtn);                 /* 画按钮 */
    }

    while (rval == 0)
    {
        tp_dev.scan(0);
        in_obj.get_key(&tp_dev, IN_TYPE_TOUCH); /* 得到按键键值 */
        delay_ms(5);

        if (system_task_return)break;           /* TPAD返回 */

        filelistbox_check(flistbox, &in_obj);   /* 扫描文件 */
        res = btn_check(rbtn, &in_obj);

        if (res)
        {
            if (((rbtn->sta & 0X80) == 0))      /* 按钮状态改变了 */
            {
                if (flistbox->dbclick != 0X81)
                {
                    filelistx = filelist_search(flistbox->list, flistbox->selindex); /* 得到此时选中的list的信息 */

                    if (filelistx->type == FICO_DISK)   /* 已经不能再往上了 */
                    {
                        break;
                    }
                    else filelistbox_back(flistbox);    /* 退回上一层目录 */
                }
            }
        }

        if (flistbox->dbclick == 0X81) /* 双击文件了 */
        {
            rval = f_opendir(&nesdir, (const TCHAR *)flistbox->path); /* 打开选中的目录 */

            if (rval)break;

            dir_sdi(&nesdir, flistbox->findextbl[flistbox->selindex - flistbox->foldercnt]);
            rval = f_readdir(&nesdir, nesinfo); /* 读取文件信息 */

            if (rval)break; /* 打开成功 */

            fn = (uint8_t *)nesinfo->fname;
            pname = gui_memin_malloc(strlen((const char *)fn) + strlen((const char *)flistbox->path) + 2); /* 申请内存 */

            if (pname == NULL)rval = 1; /* 申请失败 */
            else
            {
                pname = gui_path_name(pname, flistbox->path, fn); /* 文件名加入路径 */

                if (audiodev.status & (1 << 7)) /* 当前在放歌?? */
                {
                    audio_stop_req(&audiodev);  /* 停止音频播放 */
                    audio_task_delete();        /* 删除音乐播放任务 */
                    delay_ms(1010);
                }

                lcd_clear(BLACK);
                app_filebrower(fn, 0X05);   /* 显示当前正在玩的游戏名字 */

                /* SRAM设置 */
                FSMC_Bank1->BTCR[5] &= 0XFFFF00FF;  /* 清除之前的设置 */
                FSMC_Bank1->BTCR[5] |= 7 << 8;      /* 数据保持时间（DATAST）为8个HCLK 8/128M=62.5ns */
                USART3->CR1 &= ~(1 << 5);           /* 禁止串口3接收（此时不再处理GSM相关信息） */
                
                /* 开始nes游戏 */
                res = nes_load(pname);              /* 开始nes游戏 */

                if (res)
                {
                    window_msg_box((lcddev.width - 220) / 2, (lcddev.height - 100) / 2, 220, 100, (uint8_t *)nes_errormsg_tbl[res - 1][gui_phy.language], (uint8_t *)APP_REMIND_CAPTION_TBL[gui_phy.language], 12, 0, 0, 0);
                    delay_ms(1200);
                }

                /* SRAM设置 */
                FSMC_Bank1->BTCR[5] &= 0XFFFF00FF;      /* 清除之前的设置 */
                FSMC_Bank1->BTCR[5] |= 2 << 8;          /* 数据保持时间（DATAST）为7个HCLK 7/128M=55ns */

                system_task_return = 0;                 /* 退出标志清零 */
                
                usart3_init(36, 115200);                /* 恢复串口3功能 */
            }

            flistbox->dbclick = 0;      /* 设置非文件浏览状态 */
            gui_memin_free(pname);      /* 释放内存 */
            pname = NULL;
            app_filebrower((uint8_t *)nes_caption_tbl[gui_phy.language], 0X07); /* 选择目标文件,并得到目标数量 */
            btn_draw(rbtn);/* 画按钮 */
            delay_ms(100);
            
            filelistbox_rebuild_filelist(flistbox);/* 重建flistbox */
            system_task_return = 0; /* 刚刚退出正在玩的游戏,还不能退出这个循环 */
            tpad_init(6);           /* 重新设置TPAD值 */
        }
    }

    filelistbox_delete(flistbox);   /* 删除filelist */
    btn_delete(rbtn);               /* 删除按钮 */
    gui_memin_free(pname);          /* 释放内存 */
    gui_memin_free(nesinfo);
    return rval;
}























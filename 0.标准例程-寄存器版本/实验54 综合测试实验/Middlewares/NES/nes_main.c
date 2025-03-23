/**
 ****************************************************************************************************
 * @file        nes_main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-10-29
 * @brief       NES主函数 代码
 *              本程序移植自网友ye781205的NES模拟器工程, 特此感谢!
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
 * V1.1 20220531
 * 1, 修改注释方式
 * 2, 修改u8/u16/u32为uint8_t/uint16_t/uint32_t
 ****************************************************************************************************
 */

#include "audioplay.h"
#include "spb.h"
#include "string.h" 
#include "nes_main.h"
#include "nes_ppu.h"
#include "nes_mapper.h"
#include "nes_apu.h"

#include "./MALLOC/malloc.h"
#include "./BSP/KEY/key.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/TIMER/timer.h"
#include "./BSP/JOYPAD/joypad.h"
#include "./BSP/VS10XX/vs10xx.h"
#include "./BSP/SPI/spi.h"

#include "./FATFS/source/ff.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"




extern volatile uint8_t framecnt;   /* 帧计数器 */
int MapperNo;           /* map编号 */
int NES_scanline;       /* nes扫描线 */
int VROM_1K_SIZE;
int VROM_8K_SIZE;
uint32_t NESrom_crc32;

uint8_t PADdata0;       /* 手柄1键值 [7:0]右7 左6 下5 上4 Start3 Select2 B1 A0 */
uint8_t PADdata1;       /* 手柄2键值 [7:0]右7 左6 下5 上4 Start3 Select2 B1 A0 */
uint8_t *NES_RAM;       /* 保持1024字节对齐 */
uint8_t *NES_SRAM;
NES_header *RomHeader;  /* rom文件头 */
MAPPER *NES_Mapper;
MapperCommRes *MAPx;


uint8_t *spr_ram;       /* 精灵RAM,256字节 */
ppu_data *ppu;          /* ppu指针 */
uint8_t *VROM_banks;
uint8_t *VROM_tiles;

apu_t *apu;             /* apu指针 */
uint8_t *wave_buffers;


uint8_t *nesapusbuf[NES_APU_BUF_NUM];   /* 音频缓冲帧 */
uint8_t *romfile;                       /* nes文件指针,指向整个nes文件的起始地址 */


/**
 * @brief       加载ROM
 * @param       无
 * @retval      0,成功
 *              1,内存错误 
 *              3,map错误
 */
uint8_t nes_load_rom(void)
{
    uint8_t *p;
    uint8_t i;
    uint8_t res = 0;
    p = (uint8_t *)romfile;

    if (strncmp((char *)p, "NES", 3) == 0)
    {
        RomHeader->ctrl_z = p[3];
        RomHeader->num_16k_rom_banks = p[4];
        RomHeader->num_8k_vrom_banks = p[5];
        RomHeader->flags_1 = p[6];
        RomHeader->flags_2 = p[7];

        if (RomHeader->flags_1 & 0x04)p += 512; /* 有512字节的trainer */

        if (RomHeader->num_8k_vrom_banks > 0)   /* 存在VROM,进行预解码 */
        {
            VROM_banks = p + 16 + (RomHeader->num_16k_rom_banks * 0x4000);
#if NES_RAM_SPEED == 1  /* 1:内存占用小 0:速度快 */
            VROM_tiles = VROM_banks;
#else
            VROM_tiles = mymalloc(SRAMEX, RomHeader->num_8k_vrom_banks * 8 * 1024); /* 这里可能申请多达1MB内存!!! */

            if (VROM_tiles == 0)VROM_tiles = VROM_banks;                            /* 内存不够用的情况下,尝试VROM_titles与VROM_banks共用内存 */

            compile(RomHeader->num_8k_vrom_banks * 8 * 1024 / 16, VROM_banks, VROM_tiles);
#endif
        }
        else
        {
            VROM_banks = mymalloc(SRAMIN, 8 * 1024);
            VROM_tiles = mymalloc(SRAMEX, 8 * 1024);

            if (!VROM_banks || !VROM_tiles)res = 1;
        }

        VROM_1K_SIZE = RomHeader->num_8k_vrom_banks * 8;
        VROM_8K_SIZE = RomHeader->num_8k_vrom_banks;
        MapperNo = (RomHeader->flags_1 >> 4) | (RomHeader->flags_2 & 0xf0);

        if (RomHeader->flags_2 & 0x0E)MapperNo = RomHeader->flags_1 >> 4;   /* 忽略高四位，如果头看起来很糟糕 */

        printf("use map:%d\r\n", MapperNo);

        for (i = 0; i < 255; i++)   /*  查找支持的Mapper号 */
        {
            if (MapTab[i] == MapperNo)break;

            if (MapTab[i] == -1)res = 3;
        }

        if (res == 0)
        {
            switch (MapperNo)
            {
                case 1:
                    MAP1 = mymalloc(SRAMIN, sizeof(Mapper1Res));

                    if (!MAP1)res = 1;

                    break;

                case 4:
                case 6:
                case 16:
                case 17:
                case 18:
                case 19:
                case 21:
                case 23:
                case 24:
                case 25:
                case 64:
                case 65:
                case 67:
                case 69:
                case 85:
                case 189:
                    MAPx = mymalloc(SRAMIN, sizeof(MapperCommRes));

                    if (!MAPx)res = 1;

                    break;

                default:
                    break;
            }
        }
    }

    return res; /* 返回执行结果 */
}

/**
 * @brief       释放内存
 * @param       无
 * @retval      无
 */
void nes_sram_free(void)
{
    uint8_t i;
    myfree(SRAMIN, NES_RAM);
    myfree(SRAMIN, NES_SRAM);
    myfree(SRAMIN, RomHeader);
    myfree(SRAMIN, NES_Mapper);
    myfree(SRAMIN, spr_ram);
    myfree(SRAMIN, ppu);
    myfree(SRAMIN, apu);
    myfree(SRAMIN, wave_buffers);

    for (i = 0; i < NES_APU_BUF_NUM; i++)myfree(SRAMEX, nesapusbuf[i]); /* 释放APU BUFs */

    myfree(SRAMEX, romfile);

    if ((VROM_tiles != VROM_banks) && VROM_banks && VROM_tiles) /* 如果分别为VROM_banks和VROM_tiles申请了内存,则释放 */
    {
        myfree(SRAMIN, VROM_banks);
        myfree(SRAMEX, VROM_tiles);
    }

    switch (MapperNo)       /* 释放map内存 */
    {
        case 1:             /* 释放内存 */
            myfree(SRAMIN, MAP1);
            break;

        case 4:
        case 6:
        case 16:
        case 17:
        case 18:
        case 19:
        case 21:
        case 23:
        case 24:
        case 25:
        case 64:
        case 65:
        case 67:
        case 69:
        case 85:
        case 189:
            myfree(SRAMIN, MAPx);
            break;          /* 释放内存 */

        default:
            break;
    }

    NES_RAM = 0;
    NES_SRAM = 0;
    RomHeader = 0;
    NES_Mapper = 0;
    spr_ram = 0;
    ppu = 0;
    apu = 0;
    wave_buffers = 0;

    for (i = 0; i < NES_APU_BUF_NUM; i++)
    {
        nesapusbuf[i] = 0;
    }
    
    VROM_banks = 0;
    VROM_tiles = 0;
    MAP1 = 0;
    MAPx = 0;
}

/**
 * @brief       为NES运行申请内存
 * @param       romsize         : nes文件大小
 * @retval      0,申请成功
 *              1,申请失败
 */
uint8_t nes_sram_malloc(uint32_t romsize)
{
    uint16_t i = 0;

    for (i = 0; i < 64; i++)    /* 为NES_RAM,查找1024对齐的内存 */
    {
        NES_SRAM = mymalloc(SRAMIN, i * 32);
        NES_RAM = mymalloc(SRAMIN, 0X800);  /* 申请2K字节,必须1024字节对齐 */

        if ((uint32_t)NES_RAM % 1024)       /* 不是1024字节对齐 */
        {
            myfree(SRAMIN, NES_RAM);        /* 释放内存,然后重新尝试分配 */
            myfree(SRAMIN, NES_SRAM);
        }
        else
        {
            myfree(SRAMIN, NES_SRAM);       /* 释放内存 */
            break;
        }
    }

    NES_SRAM = mymalloc(SRAMIN, 0X2000);
    RomHeader = mymalloc(SRAMIN, sizeof(NES_header));
    NES_Mapper = mymalloc(SRAMIN, sizeof(MAPPER));
    spr_ram = mymalloc(SRAMIN, 0X100);
    ppu = mymalloc(SRAMIN, sizeof(ppu_data));
    apu = mymalloc(SRAMIN, sizeof(apu_t));  /* sizeof(apu_t)=  12588 */
    wave_buffers = mymalloc(SRAMIN, APU_PCMBUF_SIZE);

    for (i = 0; i < NES_APU_BUF_NUM; i++)
    {
        nesapusbuf[i] = mymalloc(SRAMEX, APU_PCMBUF_SIZE + 10); /* 申请内存 */
    }

    romfile = mymalloc(SRAMEX, romsize);    /* 申请游戏rom空间,等于nes文件大小 */

    if (romfile == NULL)    /* 内存不够?释放主界面占用内存,再重新申请 */
    {
        spb_delete();       /* 释放SPB占用的内存 */
        romfile = mymalloc(SRAMEX, romsize);    /* 重新申请 */
    }

    if (i == 64 || !NES_RAM || !NES_SRAM || !RomHeader || !NES_Mapper || !spr_ram || !ppu || !apu || !wave_buffers || !nesapusbuf[NES_APU_BUF_NUM - 1] || !romfile)
    {
        nes_sram_free();
        return 1;
    }

    memset(NES_SRAM, 0, 0X2000);                /* 清零 */
    memset(RomHeader, 0, sizeof(NES_header));   /* 清零 */
    memset(NES_Mapper, 0, sizeof(MAPPER));      /* 清零 */
    memset(spr_ram, 0, 0X100);                  /* 清零 */
    memset(ppu, 0, sizeof(ppu_data));           /* 清零 */
    memset(apu, 0, sizeof(apu_t));              /* 清零 */
    memset(wave_buffers, 0, APU_PCMBUF_SIZE);   /* 清零 */

    for (i = 0; i < NES_APU_BUF_NUM; i++)memset(nesapusbuf[i], 0, APU_PCMBUF_SIZE + 10); /* 清零 */

    memset(romfile, 0, romsize);                /* 清零 */
    return 0;
}

/**
 * @brief       频率设置
 * @param       plln            : 倍频数
 * @retval      无
 */
void nes_clock_set(uint8_t plln)
{
    uint16_t tpll = plln;
    uint8_t temp = 0;
    
    RCC->CFGR &= 0XFFFFFFFC;    /* 修改时钟频率为内部8M */
    RCC->CR &= ~0x01000000;     /* PLL OFF */
    RCC->CFGR &= ~(0XF << 18);  /* 清空原来的设置 */
    plln -= 2;                  /* 抵消2个单位 */
    
    RCC->CFGR |= plln << 18;    /* 设置PLL值 2~16 */
    RCC->CFGR |= 1 << 16;       /* PLLSRC ON */
    
    if(tpll > 9)                /* 大于72M的时候 */
    {
        FLASH->ACR = 0x13;      /* FLASH 3个延时周期 */
    }else
    {
        FLASH->ACR = 0x12;      /* FLASH 2个延时周期 */
    }
    
    RCC->CR |= 0x01000000;      /* PLLON */

    while (!(RCC->CR >> 25));   /* 等待PLL锁定 */

    RCC->CFGR |= 0x02;          /* PLL作为系统时钟 */

    while (temp != 0x02)        /* 等待PLL作为系统时钟设置成功 */
    {
        temp = RCC->CFGR >> 2;
        temp &= 0x03;
    }

    /* 顺便设置延时和串口 */
    delay_init(tpll * 8);           /* 延时初始化 */
    usart_init(tpll * 8, 115200);   /* 串口1初始化 */
}

extern volatile uint8_t nes_spped_para; /* NES游戏进行时,将会对此值设置,默认为0 */

/**
 * @brief       开始nes游戏
 * @param       pname           : nes游戏路径
 * @retval      0,正常退出
 *              1,内存错误
 *              2,文件错误
 *              3,不支持的map
 */
uint8_t nes_load(uint8_t *pname)
{
    FIL *file;
    UINT br;
    uint8_t res = 0;
    file = mymalloc(SRAMIN, sizeof(FIL));

    if (file == 0)return 1; /* 内存申请失败 */

    res = f_open(file, (char *)pname, FA_READ);

    if (res != FR_OK)       /* 打开文件失败 */
    {
        myfree(SRAMIN, file);
        return 2;
    }

    res = nes_sram_malloc(file->obj.objsize); /* 申请内存 */

    if (res == 0)
    {
        f_read(file, romfile, file->obj.objsize, &br);                  /* 读取nes文件 */
        NESrom_crc32 = get_crc32(romfile + 16, file->obj.objsize - 16); /* 获取CRC32的值 */
        res = nes_load_rom();       /* 加载ROM */

        if (res == 0)
        {
            nes_clock_set(16);      /* 超频到128M,如果部分MCU出现死机,可以考虑降低超频 */
            tpad_init(16);          /* 避免错误退出, 把分频搞大点 */
            
            nes_spped_para = 1;     /* SPI速度减半 */
           
            joypad_init();
            cpu6502_init();         /* 初始化6502,并复位 */
            Mapper_Init();          /* map初始化 */
            PPU_reset();            /* ppu复位 */
            apu_init();             /* apu初始化 */
            
            nes_sound_open(0, APU_SAMPLE_RATE); /* 初始化播放设备 */
            
            nes_emulate_frame();    /* 进入NES模拟器主循环 */
            nes_sound_close();      /* 关闭声音输出 */
            nes_clock_set(9);       /* 72M */
            tpad_init(6);
            
            nes_spped_para = 0;     /* SPI速度恢复 */
        }
    }

    f_close(file);
    myfree(SRAMIN, file);   /* 释放内存 */
    nes_sram_free();        /* 释放内存 */
    return res;
}

uint8_t nes_xoff = 0;       /* 显示在x轴方向的偏移量(实际显示宽度=256-2*nes_xoff) */

/**
 * @brief       设置游戏显示窗口
 * @param       无
 * @retval      无
 */
void nes_set_window(void)
{
    uint16_t xoff = 0, yoff = 0;
    uint16_t lcdwidth, lcdheight;

    if (lcddev.width == 240)
    {
        lcdwidth = 240;
        lcdheight = 240;
        nes_xoff = (256 - lcddev.width) / 2;        /* 得到x轴方向的偏移量 */
        xoff = 0;
    }
    else if (lcddev.width == 320)
    {
        lcdwidth = 256;
        lcdheight = 240;
        nes_xoff = 0;
        xoff = (lcddev.width - 256) / 2;
    }
    else if (lcddev.width == 480)
    {
        lcdwidth = 480;
        lcdheight = 480;
        nes_xoff = (256 - (lcddev.width / 2)) / 2;  /* 得到x轴方向的偏移量 */
        xoff = 0;
    }

    yoff = (lcddev.height - lcdheight) / 2;         /* 屏幕高度 */
    lcd_set_window(xoff, yoff, lcdwidth, lcdheight);/* 让NES始终在屏幕的正中央显示 */
    lcd_write_ram_prepare();                        /* 写入LCD RAM的准备 */
}

extern void KEYBRD_FCPAD_Decode(uint8_t *fcbuf, uint8_t mode);

/**
 * @brief       读取游戏手柄数据
 * @param       无
 * @retval      无
 */
void nes_get_gamepadval(void)
{
    PADdata0 = joypad_read();   /* 读取手柄1的值 */
    //PADdata1=0;               /* 没有手柄2,故不采用 */
}

/**
 * @brief       nes模拟器主循环
 * @param       无
 * @retval      无
 */
void nes_emulate_frame(void)
{
    uint8_t nes_frame;
    tim3_int_init(10000 - 1, 12800 - 1);    /* 启动TIM3 ,1s中断一次 */
    nes_set_window();                       /* 设置窗口 */
    system_task_return = 0;

    while (1)
    {
        /*  LINES 0-239 */
        PPU_start_frame();

        for (NES_scanline = 0; NES_scanline < 240; NES_scanline++)
        {
            run6502(113 * 256);
            NES_Mapper->HSync(NES_scanline);

            /* 扫描一行 */
            if (nes_frame == 0)scanline_draw(NES_scanline);
            else do_scanline_and_dont_draw(NES_scanline);
        }

        NES_scanline = 240;
        run6502(113 * 256); /* 运行1线 */
        NES_Mapper->HSync(NES_scanline);
        start_vblank();

        if (NMI_enabled())
        {
            cpunmi = 1;
            run6502(7 * 256); /* 运行中断 */
        }

        NES_Mapper->VSync();

        /*  LINES 242-261 */
        for (NES_scanline = 241; NES_scanline < 262; NES_scanline++)
        {
            run6502(113 * 256);
            NES_Mapper->HSync(NES_scanline);
        }

        end_vblank();
        nes_get_gamepadval();   /* 每3帧查询一次USB */
        apu_soundoutput();      /* 输出游戏声音 */
        framecnt++;
        nes_frame++;

        if (nes_frame > NES_SKIP_FRAME)
        {
            nes_frame = 0;      /* 跳帧 */

            if (lcddev.id == 0X1963)nes_set_window();   /* 重设窗口 */
        }

        if (system_task_return)
        {
            system_task_return = 0;
            break;
        }
    }

    lcd_set_window(0, 0, lcddev.width, lcddev.height);  /* 恢复屏幕窗口 */
    TIM3->CR1 &= ~(1 << 0); /* 关闭定时器3 */
}

/* 在6502.s里面被调用 */
void debug_6502(uint16_t reg0, uint8_t reg1)
{
    printf("6502 error:%x,%d\r\n", reg0, reg1);
}

/* nes,音频输出支持部分 */
volatile uint16_t nesbufpos = 0;    /* 当前缓冲编号 */
volatile uint8_t nesplaybuf = 0;    /* 即将播放的音频帧缓冲编号 */
volatile uint8_t nessavebuf = 0;    /* 当前保存到的音频缓冲编号 */

/**
 * @brief       音频播放回调函数
 * @param       无
 * @retval      无
 */
void nes_vs10xx_feeddata(void)
{
    uint8_t n;
    uint8_t nbytes;
    uint8_t *p;

    if (nesplaybuf == nessavebuf)return;    /* 还没有收到新的音频数据 */

    if (VS10XX_DQ != 0) /* 可以发送数据给VS10XX */
    {
        p = nesapusbuf[nesplaybuf] + nesbufpos;
        nesbufpos += 32;

        if (nesbufpos > APU_PCMBUF_SIZE)
        {
            nesplaybuf++;

            if (nesplaybuf > (NES_APU_BUF_NUM - 1))nesplaybuf = 0;

            nbytes = APU_PCMBUF_SIZE + 32 - nesbufpos;
            nesbufpos = 0;
        }
        else nbytes = 32;

        for (n = 0; n < nbytes; n++)
        {
            if (p[n] != 0)break;    /* 判断是不是剩余所有的数据都为0? */
        }

        if (n == nbytes)return;     /* 都是0,则直接不写入VS1053了,以免引起哒哒声 */

        VS10XX_XDCS(0);

        for (n = 0; n < nbytes; n++)
        {
            vs10xx_spi_read_write_byte(p[n]);
        }

        VS10XX_XDCS(1);
    }
}

/* NES模拟器声音从VS1053输出,模拟WAV解码的wav头数据 */
const uint8_t nes_wav_head[] =
{
    0X52, 0X49, 0X46, 0X46, 0XFF, 0XFF, 0XFF, 0XFF, 0X57, 0X41, 0X56, 0X45, 0X66, 0X6D, 0X74, 0X20,
    0X10, 0X00, 0X00, 0X00, 0X01, 0X00, 0X01, 0X00, 0X11, 0X2B, 0X00, 0X00, 0X11, 0X2B, 0X00, 0X00,
    0X01, 0X00, 0X08, 0X00, 0X64, 0X61, 0X74, 0X61, 0XFF, 0XFF, 0XFF, 0XFF, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
};

/**
 * @brief       NES打开音频输出
 * @param       samples_per_sync: 样本同步(未用到)
 * @param       sample_rate     : 音频采样率
 * @retval      0, 成功;
 *              其他, 失败;
 */
int nes_sound_open(int samples_per_sync, int sample_rate)
{
    uint8_t *p;
    uint8_t i;
    p = mymalloc(SRAMIN, 100);  /* 申请100字节内存 */

    if (p == NULL)return 1;     /* 内存申请失败,直接退出 */

    printf("sound open:%d\r\n", sample_rate);

    for (i = 0; i < sizeof(nes_wav_head); i++)  /* 复制nes_wav_head内容 */
    {
        p[i] = nes_wav_head[i];
    }

    if (lcddev.width == 480)    /* 是480*480屏幕 */
    {
        sample_rate = 8000;     /* 设置8Khz,约原来速度的0.75倍 */
    }
 
    p[24] = sample_rate & 0XFF; /* 设置采样率 */
    p[25] = (sample_rate >> 8) & 0XFF;
    p[28] = sample_rate & 0XFF; /* 设置字节速率(8位模式,等于采样率) */
    p[29] = (sample_rate >> 8) & 0XFF;
    nesplaybuf = 0;
    nessavebuf = 0;

    vs10xx_reset();             /* 硬复位 */
    vs10xx_soft_reset();        /* 软复位 */
    vs10xx_set_all();           /* 设置音量等参数 */
    vs10xx_reset_decode_time(); /* 复位解码时间 */

    while (vs10xx_send_music_data(p));      /* 发送wav head */

    while (vs10xx_send_music_data(p + 32)); /* 发送wav head */

    tim6_int_init(100 - 1, 1280 - 1);       /* 1ms中断一次 */

    myfree(SRAMIN, p);          /* 释放内存 */
    return 0;
}

/**
 * @brief       NES关闭音频输出
 * @param       无
 * @retval      无
 */
void nes_sound_close(void)
{
    TIM6->CR1 &= ~(1 << 0); /* 关闭定时器6 */
    vs10xx_set_speaker(0);  /* 关闭喇叭输出 */
    vs10xx_set_volume(0);   /* 设置音量为0 */
}

/**
 * @brief       NES音频输出到VS1053缓存
 * @param       samples         : 样本(未用到)
 * @param       wavebuf         : 音频数据缓冲
 * @retval      无
 */
void nes_apu_fill_buffer(int samples, uint8_t *wavebuf)
{
    uint16_t i;
    uint8_t tbuf;

    for (i = 0; i < APU_PCMBUF_SIZE; i++)
    {
        nesapusbuf[nessavebuf][i] = wavebuf[i];
    }

    tbuf = nessavebuf;
    tbuf++;

    if (tbuf > (NES_APU_BUF_NUM - 1))tbuf = 0;

    while (tbuf == nesplaybuf)  /* 输出数据赶上音频播放的位置了,等待 */
    {
        delay_ms(5);
    }

    nessavebuf = tbuf;
}




















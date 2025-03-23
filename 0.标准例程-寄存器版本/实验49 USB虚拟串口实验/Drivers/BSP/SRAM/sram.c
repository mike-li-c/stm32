/**
 ****************************************************************************************************
 * @file        sram.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-27
 * @brief       外部SRAM 驱动代码
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
 * 修改说明
 * V1.0 20200427
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/SRAM/sram.h"
#include "./SYSTEM/usart/usart.h"


/**
 * @brief       初始化 外部SRAM
 * @param       无
 * @retval      无
 */
void sram_init(void)
{
    SRAM_CS_GPIO_CLK_ENABLE();  /* SRAM_CS脚时钟使能 */
    SRAM_WR_GPIO_CLK_ENABLE();  /* SRAM_WR脚时钟使能 */
    SRAM_RD_GPIO_CLK_ENABLE();  /* SRAM_RD脚时钟使能 */

    RCC->APB2ENR |= 0XF << 5;   /* 使能PD,PE,PF,PG时钟 */
    RCC->AHBENR |= 1 << 8;      /* 使能FSMC时钟 */

    sys_gpio_set(SRAM_CS_GPIO_PORT, SRAM_CS_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* SRAM_CS引脚模式设置 */

    sys_gpio_set(SRAM_WR_GPIO_PORT, SRAM_WR_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* SRAM_WR引脚模式设置 */

    sys_gpio_set(SRAM_RD_GPIO_PORT, SRAM_RD_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* SRAM_RD引脚模式设置 */

    /* SRAM_D0 ~ SRAM_D15 IO口初始化 */
    sys_gpio_set(GPIOD, (3 << 0) | (7 << 8) | (3 << 14), SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* PD0,1,8,9,10,14,15   AF OUT */
    sys_gpio_set(GPIOE, (0X1FF << 7), SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);                      /* PE7~15  AF OUT */

    /* SRAM_A0 ~ SRAM_A18 IO口初始化
     * 我们使用的SRAM芯片为: XM8A51216 / IS62WV51216, 容量为 512K * 16bit, 即1M字节
     * 总共需要19根数据线 容量即: 2^19 * 16bit
     */
    sys_gpio_set(GPIOF, (0X3F << 0) | (0XF << 12) , SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);        /* PF0~5, PF12~15  AF OUT */
    sys_gpio_set(GPIOG, (0X3F << 0), SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);                       /* PG0~5  AF OUT */
    sys_gpio_set(GPIOD, (0X07 << 11), SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);                      /* PD11, 12, 13  AF OUT */

    /* NBL0, NBL1, 高低字节选择 IO初始化 */
    sys_gpio_set(GPIOE, (0X03 << 0), SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);                       /* PE0, 1  AF OUT */

    /* FSMC时钟来自HCLK, 频率为72Mhz
     * 寄存器清零
     * bank1有NE1~4,每一个有一个BCR+TCR，所以总共八个寄存器。
     * 这里我们使用NE3 ，也就对应BTCR[4], [5]
     */
    SRAM_FSMC_BCRX = 0X00000000;    /* BCR寄存器清零 */
    SRAM_FSMC_BTRX = 0X00000000;    /* BTR寄存器清零 */
    SRAM_FSMC_BWTRX = 0X00000000;   /* BWTR寄存器清零 */

    /* 设置BCR寄存器 使用异步模式 */
    SRAM_FSMC_BCRX |= 1 << 12;      /* 存储器写使能 */
    SRAM_FSMC_BCRX |= 0 << 14;      /* 读写使用相同的时序 */
    SRAM_FSMC_BCRX |= 1 << 4;       /* 存储器数据宽度为16bit */

    /* 设置BTR寄存器, 读时序控制寄存器 */
    SRAM_FSMC_BTRX |= 0 << 28;      /* 模式A */
    SRAM_FSMC_BTRX |= 2 << 8;       /* 数据保持时间(DATAST)为3个HCLK 1/72M * 3 = 41.7ns (对IS62WV51216, 较慢) */
    SRAM_FSMC_BTRX |= 0 << 0;       /* 地址建立时间(ADDSET)为1个HCLK 1/72M = 13.9ns (实际 > 200ns) */

    /* 使能BANK1,区域1 */
    SRAM_FSMC_BCRX |= 1 << 0;       /* 使能BANK1，区域1 */
    SRAM_FSMC_BCRX |= (uint32_t)1 << 31;    /* 使能FSMC */
}

/**
 * @brief       往SRAM指定地址写入指定长度数据
 * @param       pbuf    : 数据存储区
 * @param       addr    : 开始写入的地址(最大32bit)
 * @param       datalen : 要写入的字节数(最大32bit)
 * @retval      无
 */
void sram_write(uint8_t *pbuf, uint32_t addr, uint32_t datalen)
{
    for (; datalen != 0; datalen--)
    {
        *(volatile uint8_t *)(SRAM_BASE_ADDR + addr) = *pbuf;
        addr++;
        pbuf++;
    }
}

/**
 * @brief       从SRAM指定地址读取指定长度数据
 * @param       pbuf    : 数据存储区
 * @param       addr    : 开始读取的地址(最大32bit)
 * @param       datalen : 要读取的字节数(最大32bit)
 * @retval      无
 */
void sram_read(uint8_t *pbuf, uint32_t addr, uint32_t datalen)
{
    for (; datalen != 0; datalen--)
    {
        *pbuf++ = *(volatile uint8_t *)(SRAM_BASE_ADDR + addr);
        addr++;
    }
}

/**
 * @brief       测试函数 在SRAM指定地址写入1个字节
 * @param       addr    : 开始写入的地址(最大32bit)
 * @param       data    : 要写入的字节
 * @retval      无
 */
void sram_test_write(uint32_t addr, uint8_t data)
{
    sram_write(&data, addr, 1);     /* 写入1个字节 */
}

/**
 * @brief       测试函数 在SRAM指定地址读取1个字节
 * @param       addr    : 开始读取的地址(最大32bit)
 * @retval      读取到的数据(1个字节)
 */
uint8_t sram_test_read(uint32_t addr)
{
    uint8_t data;
    sram_read(&data, addr, 1);      /* 读取1个字节 */
    return data;
}













/**
 ****************************************************************************************************
 * @file        joypad.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-26
 * @brief       游戏手柄 驱动代码
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
 * V1.0 20200426
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include  "./BSP/JOYPAD/joypad.h"


/**
 * @brief       初始化手柄接口
 * @param       无
 * @retval      无
 */
void joypad_init(void)
{
    JOYPAD_CLK_GPIO_CLK_ENABLE();   /* CLK   所在IO时钟初始化 */
    JOYPAD_LAT_GPIO_CLK_ENABLE();   /* LATCH 所在IO时钟初始化 */
    JOYPAD_DATA_GPIO_CLK_ENABLE();  /* DATA  所在IO时钟初始化 */
    
    sys_gpio_set(JOYPAD_CLK_GPIO_PORT, JOYPAD_CLK_GPIO_PIN,
                 SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_MID, SYS_GPIO_PUPD_PU);   /* JOYPAD_CLK  引脚模式设置 */

    sys_gpio_set(JOYPAD_LAT_GPIO_PORT, JOYPAD_LAT_GPIO_PIN,
                 SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_MID, SYS_GPIO_PUPD_PU);   /* JOYPAD_LAT  引脚模式设置 */

    sys_gpio_set(JOYPAD_DATA_GPIO_PORT, JOYPAD_DATA_GPIO_PIN,
                 SYS_GPIO_MODE_IN, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_MID, SYS_GPIO_PUPD_PU);    /* JOYPAD_DATA 引脚模式设置 */
}

/**
 * @brief       手柄延迟函数
 * @param       t    : 要延时的时间
 * @retval      无
 */
static void joypad_delay(uint16_t t)
{
    while (t--);
}

/**
 * @brief       读取手柄按键值
 *   @note      FC手柄数据输出格式:
 *              每给一个脉冲,输出一位数据,输出顺序:
 *              A -> B -> SELECT -> START -> UP -> DOWN -> LEFT -> RIGHT.
 *              总共8位, 对于有C按钮的手柄, 按下C其实就等于 A + B 同时按下.
 *              按下是1,松开是0.
 * @param       无
 * @retval      按键结果, 格式如下:
 *              [7]:右
 *              [6]:左
 *              [5]:下
 *              [4]:上
 *              [3]:Start
 *              [2]:Select
 *              [1]:B
 *              [0]:A
 */
uint8_t joypad_read(void)
{
    volatile uint8_t temp = 0;
    uint8_t t;
    JOYPAD_LAT(1);          /* 锁存当前状态 */
    joypad_delay(80);
    JOYPAD_LAT(0);

    for (t = 0; t < 8; t++) /* 移位输出数据 */
    {
        temp >>= 1;

        if (JOYPAD_DATA == 0)
        {
            temp |= 0x80;   /* LOAD之后，就得到第一个数据 */
        }
        
        JOYPAD_CLK(1);      /* 每给一次脉冲，收到一个数据 */
        joypad_delay(80);
        JOYPAD_CLK(0);
        joypad_delay(80);
    }

    return temp;
}




















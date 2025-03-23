/**
 ****************************************************************************************************
 * @file        pwmdac.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-24
 * @brief       PWM DAC输出 驱动代码
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
 * V1.0 20200424
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/PWMDAC/pwmdac.h"

/**
 * @brief       PWM DAC初始化, 实际上就是初始化定时器
 * @note
 *              定时器的时钟来自APB1 / APB2, 当APB1 / APB2 分频时, 定时器频率自动翻倍
 *              所以, 一般情况下, 我们所有定时器的频率, 都是72Mhz 等于系统时钟频率
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft = 定时器工作频率, 单位: Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void pwmdac_init(uint16_t arr, uint16_t psc)
{
    uint8_t chy = PWMDAC_TIMX_CHY;
    PWMDAC_GPIO_CLK_ENABLE();       /* PWM DAC GPIO 时钟使能 */
    PWMDAC_TIMX_CLK_ENABLE();       /* PWM DAC 定时器时钟使能 */

    sys_gpio_set(PWMDAC_GPIO_PORT, PWMDAC_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* TIMX PWM CHY 引脚模式设置 */

    PWMDAC_TIMX->ARR = arr;         /* 设定计数器自动重装值 */
    PWMDAC_TIMX->PSC = psc;         /* 设置预分频器  */
    PWMDAC_TIMX->BDTR |= 1 << 15;   /* 使能MOE位(仅TIM1/8 有此寄存器,必须设置MOE才能输出PWM), 其他通用定时器, 这个
                                     * 寄存器是无效的, 所以设置/不设置并不影响结果, 为了兼容这里统一改成设置MOE位
                                     */

    if (chy <= 2)
    {
        PWMDAC_TIMX->CCMR1 |= 6 << (4 + 8 * (chy - 1)); /* CH1/2 PWM模式1 */
        PWMDAC_TIMX->CCMR1 |= 1 << (3 + 8 * (chy - 1)); /* CH1/2 预装载使能 */
    }
    else if (chy <= 4)
    {
        PWMDAC_TIMX->CCMR2 |= 6 << (4 + 8 * (chy - 3)); /* CH3/4 PWM模式1 */
        PWMDAC_TIMX->CCMR2 |= 1 << (3 + 8 * (chy - 3)); /* CH3/4 预装载使能 */
    }

    PWMDAC_TIMX->CCER |= 1 << (4 * (chy - 1));      /* OCy 输出使能 */
    PWMDAC_TIMX->CCER |= 0 << (1 + 4 * (chy - 1));  /* OCy 高电平有效 */
    PWMDAC_TIMX->CR1 |= 1 << 7;     /* ARPE使能 */
    PWMDAC_TIMX->CR1 |= 1 << 0;     /* 使能定时器TIMX */
}

/**
 * @brief       设置PWM DAC输出电压
 * @param       vol : 0~3300,代表0~3.3V
 * @retval      无
 */
void pwmdac_set_voltage(uint16_t vol)
{
    float temp = vol;
    temp /= 100;                /* 缩小100倍, 得到实际电压值 */
    temp = temp * 256 / 3.3;    /* 将电压转换成PWM占空比 */
    PWMDAC_TIMX_CCRX = temp;    /* 设置新的占空比 */
}









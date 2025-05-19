/**
 ****************************************************************************************************
 * @file        atim.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2020-04-21
 * @brief       高级定时器 驱动代码
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
 * V1.0 20200421
 * 第一次发布
 * V1.1 20200421
 * 1, 新增atim_timx_comp_pwm_init函数, 实现输出比较模式PWM输出功能
 *
 ****************************************************************************************************
 */

#include "atim.h"
#include "led.h"


/* g_npwm_remain表示当前还剩下多少个脉冲要发送
 * 每次最多发送256个脉冲
 */
static uint32_t g_npwm_remain = 0;

/**
 * @brief       高级定时器TIMX NPWM中断服务函数
 * @param       无
 * @retval      无
 */
void ATIM_TIMX_NPWM_IRQHandler(void)
{
    uint16_t npwm = 0;

    if (ATIM_TIMX_NPWM->SR & 0X0001)    /* 溢出中断, 且RCR=0 */
    {
        if (g_npwm_remain > 256)        /* 还有大于256个脉冲需要发送 */
        {
            g_npwm_remain = g_npwm_remain - 256;
            npwm = 256;
        }
        else if (g_npwm_remain % 256)   /* 还有位数（不到256）个脉冲要发送 */
        {
            npwm = g_npwm_remain % 256;
            g_npwm_remain = 0;          /* 没有脉冲了 */
        }

        if (npwm)   /* 有脉冲要发送 */
        {
            ATIM_TIMX_NPWM->RCR = npwm - 1; /* 设置重复计数寄存器值为npwm-1, 即npwm个脉冲 */
            ATIM_TIMX_NPWM->EGR |= 1 << 0;  /* 产生一次更新事件,以更新RCR寄存器 */
            ATIM_TIMX_NPWM->CR1 |= 1 << 0;  /* 使能定时器TIMX */
        } 
        else
        {
            ATIM_TIMX_NPWM->CR1 &= ~(1 << 0);   /* 关闭定时器TIMX */
        }
    }

    ATIM_TIMX_NPWM->SR &= ~(1 << 0);    /* 清除中断标志位 */
}

/**
 * @brief       高级定时器TIMX 通道Y 输出指定个数PWM 初始化函数
 * @note
 *              高级定时器的时钟来自APB2, 而PCLK2 = 72Mhz, 我们设置PPRE2不分频, 因此
 *              高级定时器时钟 = 72Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void atim_timx_npwm_chy_init(uint16_t arr, uint16_t psc)
{
    uint8_t chy = ATIM_TIMX_NPWM_CHY;
    ATIM_TIMX_NPWM_CHY_GPIO_CLK_ENABLE();   /* TIMX 通道IO口时钟使能 */
    ATIM_TIMX_NPWM_CHY_CLK_ENABLE();        /* TIMX 时钟使能 */

    sys_gpio_set(ATIM_TIMX_NPWM_CHY_GPIO_PORT, ATIM_TIMX_NPWM_CHY_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* TIMX PWM CHY 引脚模式设置 */

    ATIM_TIMX_NPWM->ARR = arr;       /* 设定计数器自动重装值 */
    ATIM_TIMX_NPWM->PSC = psc;       /* 设置预分频器  */
    ATIM_TIMX_NPWM->BDTR |= 1 << 15; /* 使能MOE位, 高级定时器必须设置MOE才能输出PWM */

    if (chy <= 2)
    {
        ATIM_TIMX_NPWM->CCMR1 |= 6 << (4 + 8 * (chy - 1));  /* CH1/2 PWM模式1 */
        ATIM_TIMX_NPWM->CCMR1 |= 1 << (3 + 8 * (chy - 1));  /* CH1/2 预装载使能 */
    }
    else if (chy <= 4)
    {
        ATIM_TIMX_NPWM->CCMR2 |= 6 << (4 + 8 * (chy - 3));  /* CH3/4 PWM模式1 */
        ATIM_TIMX_NPWM->CCMR2 |= 1 << (3 + 8 * (chy - 3));  /* CH3/4 预装载使能 */
    }

    ATIM_TIMX_NPWM->CCER |= 1 << (4 * (chy - 1));       /* OCy 输出使能 */
    ATIM_TIMX_NPWM->CCER |= 0 << (1 + 4 * (chy - 1));   /* OCy 高电平有效 */

    ATIM_TIMX_NPWM->CR1 |= 1 << 7;  /* ARPE使能 */
    ATIM_TIMX_NPWM->DIER |= 1 << 0; /* 允许更新中断 */
    ATIM_TIMX_NPWM->CR1 |= 1 << 0;  /* 使能定时器TIMX */

    sys_nvic_init(1, 3, ATIM_TIMX_NPWM_IRQn, 2);/* 抢占1，子优先级3，组2 */
}

/**
 * @brief       高级定时器TIMX NPWM设置PWM个数
 * @param       rcr: PWM的个数, 1~2^32次方个
 * @retval      无
 */
void atim_timx_npwm_chy_set(uint32_t npwm)
{
    if (npwm == 0)return ;

    g_npwm_remain = npwm;               /* 保存脉冲个数 */
    ATIM_TIMX_NPWM->EGR |= 1 << 0;      /* 产生一次更新事件,在中断里面处理脉冲输出 */
    ATIM_TIMX_NPWM->CR1 |= 1 << 0;      /* 使能定时器TIMX */
}


/**
 * @brief       高级定时器TIMX 输出比较模式 初始化函数（使用输出比较模式）
 * @note
 *              配置高级定时器TIMX 4路输出比较模式PWM输出,实现50%占空比,不同相位控制
 *              注意,本例程输出比较模式,每2个计数周期才能完成一个PWM输出,因此输出频率减半
 *              另外,我们还可以开启中断在中断里面修改CCRx,从而实现不同频率/不同相位的控制
 *              但是我们不推荐这么使用,因为这可能导致非常频繁的中断,从而占用大量CPU资源
 *
 *              高级定时器的时钟来自APB2, 而PCLK2 = 72Mhz, 我们设置PPRE2不分频, 因此
 *              高级定时器时钟 = 72Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void atim_timx_comp_pwm_init(uint16_t arr, uint16_t psc)
{
    ATIM_TIMX_COMP_CH1_GPIO_CLK_ENABLE();   /* 通道1对应IO口时钟使能 */
    ATIM_TIMX_COMP_CH2_GPIO_CLK_ENABLE();   /* 通道2对应IO口时钟使能 */
    ATIM_TIMX_COMP_CH3_GPIO_CLK_ENABLE();   /* 通道3对应IO口时钟使能 */
    ATIM_TIMX_COMP_CH4_GPIO_CLK_ENABLE();   /* 通道4对应IO口时钟使能 */
    ATIM_TIMX_COMP_CLK_ENABLE();            /* 使能定时器时钟 */
    
    sys_gpio_set(ATIM_TIMX_COMP_CH1_GPIO_PORT, ATIM_TIMX_COMP_CH1_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* TIMX_CH1 引脚模式设置 */
    
    sys_gpio_set(ATIM_TIMX_COMP_CH2_GPIO_PORT, ATIM_TIMX_COMP_CH2_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* TIMX_CH2 引脚模式设置 */
    
    sys_gpio_set(ATIM_TIMX_COMP_CH3_GPIO_PORT, ATIM_TIMX_COMP_CH3_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* TIMX_CH3 引脚模式设置 */
    
    sys_gpio_set(ATIM_TIMX_COMP_CH4_GPIO_PORT, ATIM_TIMX_COMP_CH4_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* TIMX_CH4 引脚模式设置 */

    ATIM_TIMX_COMP->ARR = arr;          /* 设定计数器自动重装值 */
    ATIM_TIMX_COMP->PSC = psc;          /* 设置预分频器  */
    ATIM_TIMX_NPWM->BDTR |= 1 << 15;    /* 使能MOE位, 高级定时器必须设置MOE才能输出PWM */

    ATIM_TIMX_COMP->CCMR1 |= 3 << 4;    /* 通道1 输出比较模式 匹配时翻转 */
    ATIM_TIMX_COMP->CCMR1 |= 1 << 3;    /* 通道1 预装载使能 */

    ATIM_TIMX_COMP->CCMR1 |= 3 << 12;   /* 通道2 输出比较模式 匹配时翻转 */
    ATIM_TIMX_COMP->CCMR1 |= 1 << 11;   /* 通道2 预装载使能 */

    ATIM_TIMX_COMP->CCMR2 |= 3 << 4;    /* 通道3 输出比较模式 匹配时翻转 */
    ATIM_TIMX_COMP->CCMR2 |= 1 << 3;    /* 通道3 预装载使能 */

    ATIM_TIMX_COMP->CCMR2 |= 3 << 12;   /* 通道4 输出比较模式 匹配时翻转 */
    ATIM_TIMX_COMP->CCMR2 |= 1 << 11;   /* 通道4 预装载使能 */

    ATIM_TIMX_COMP->CCER |= 1 << 0;     /* OC1 输出使能 */ 
    ATIM_TIMX_COMP->CCER |= 1 << 4;     /* OC2 输出使能 */ 
    ATIM_TIMX_COMP->CCER |= 1 << 8;     /* OC3 输出使能 */ 
    ATIM_TIMX_COMP->CCER |= 1 << 12;    /* OC4 输出使能 */ 

    ATIM_TIMX_COMP->CR1 |= 1 << 7;      /* ARPE使能 */ 
    ATIM_TIMX_COMP->CR1 |= 1 << 0;      /* 使能定时器TIMX */ 
}
 






















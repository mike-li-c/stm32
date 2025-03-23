/**
 ****************************************************************************************************
 * @file        timer.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-10-28
 * @brief       通用定时器(服务综合测试实验) 驱动代码
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
 * V1.0 20221028
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/TIMER/timer.h"
#include "./BSP/USART3/usart3.h"
#include "./BSP/LED/led.h"
#include "ucos_ii.h"

volatile uint8_t framecnt;   /* 帧计数器 */

/**
 * @brief       定时器3中断服务程序
 * @param       无
 * @retval      无
 */
void TIM3_IRQHandler(void)
{
    OSIntEnter();

    if (TIM3->SR & 0X01) /* 是更新中断 */
    {
        if (framecnt)printf("frame:%d\r\n", framecnt); /* 打印帧率 */

        framecnt = 0;
    }

    TIM3->SR &= ~(1 << 0);      /* 清除中断标志位 */
    OSIntExit();
}

/**
 * @brief       基本定时器3中断初始化
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void tim3_int_init(uint16_t arr, uint16_t psc)
{
    RCC->APB1ENR |= 1 << 1;     /* TIM3时钟使能 */
    TIM3->ARR = arr;            /* 设定计数器自动重装值 */
    TIM3->PSC = psc;            /* 预分频器 */
    TIM3->DIER |= 1 << 0;       /* 允许更新中断 */
    TIM3->CR1 |= 0x01;          /* 使能定时器 */
    
    sys_nvic_init(1, 3, TIM3_IRQn, 2);  /* 抢占1，子优先级3，组2 */
}

extern void nes_vs10xx_feeddata(void);

/**
 * @brief       定时器6中断服务程序
 * @param       无
 * @retval      无
 */
void TIM6_IRQHandler(void)
{
    OSIntEnter();

    if (TIM6->SR & 0X01) /* 是更新中断 */
    {
        nes_vs10xx_feeddata();  /* 填充数据 */
    }

    TIM6->SR &= ~(1 << 0);      /* 清除中断标志位 */
    OSIntExit();
}

/**
 * @brief       基本定时器6中断初始化
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void tim6_int_init(uint16_t arr, uint16_t psc)
{
    RCC->APB1ENR |= 1 << 4;     /* TIM6时钟使能 */
    TIM6->ARR = arr;            /* 设定计数器自动重装值 */
    TIM6->PSC = psc;            /* 预分频器 */
    TIM6->DIER |= 1 << 0;       /* 允许更新中断 */
    TIM6->CR1 |= 0x01;          /* 使能定时器 */
    
    sys_nvic_init(1, 3, TIM6_IRQn, 2);  /* 抢占1，子优先级3，组2 */
}

/**
 * @brief       定时器7中断服务程序
 * @param       无
 * @retval      无
 */
void TIM7_IRQHandler(void)
{
    OSIntEnter();

    if (TIM7->SR & 0X01) /* 是更新中断 */
    {
        g_usart3_rx_sta |= 1 << 15; /* 标记接收完成 */
        TIM7->SR &= ~(1 << 0);      /* 清除中断标志位 */
        TIM7->CR1 &= ~(1 << 0);     /* 关闭定时器7 */
    }

    OSIntExit();
}

/**
 * @brief       基本定时器7中断初始化
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void tim7_int_init(uint16_t arr, uint16_t psc)
{
    RCC->APB1ENR |= 1 << 5;     /* TIM7时钟使能 */
    TIM7->ARR = arr;            /* 设定计数器自动重装值 */
    TIM7->PSC = psc;            /* 预分频器 */
    TIM7->DIER |= 1 << 0;       /* 允许更新中断 */
    TIM7->CR1 |= 0x01;          /* 使能定时器 */
    
    sys_nvic_init(0, 1, TIM7_IRQn, 2);  /* 抢占0，子优先级1，组2 */
}


/**
 * @brief       定时器8 CH2 PWM输出 初始化函数（使用PWM模式1）
 * @param       arr: 自动重装值
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void tim8_ch2n_pwm_init(uint16_t arr, uint16_t psc)
{
    RCC->APB2ENR |= 1 << 13;    /* TIM8时钟使能 */
    RCC->APB2ENR |= 1 << 3;     /* 使能PORTB时钟 */

    sys_gpio_set(GPIOB, SYS_GPIO_PIN0,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);    /* PB0 引脚模式设置 */

    TIM8->ARR = arr;        /* 设定计数器自动重装值 */
    TIM8->PSC = psc;        /* 设置预分频器  */
    TIM8->BDTR |= 1 << 15;  /* 使能MOE位(仅TIM1/8 有此寄存器,必须设置MOE才能输出PWM), 其他通用定时器, 这个
                                     * 寄存器是无效的, 所以设置/不设置并不影响结果, 为了兼容这里统一改成设置MOE位
                                     */
 
    TIM8->CCMR1 |= 7 << 12; /* CH2 PWM模式2 */
    TIM8->CCMR1 |= 1 << 11; /* CH2 预装载使能 */
    
    TIM8->CCER |= 1 << 6;   /* OC2互补输出使能 */
    TIM8->CCER |= 1 << 7;   /* OC2N低电平有效 */
    TIM8->CR1 |= 1 << 7;    /* ARPE使能 */
    TIM8->CR1 |= 1 << 0;    /* 使能定时器TIMX */
}









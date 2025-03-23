/**
 ****************************************************************************************************
 * @file        exti.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2020-04-26
 * @brief       外部中断 驱动代码
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
 * V1.0 20200419
 * 第一次发布
 * V1.1 20200426
 * 1, 增加exti8_init函数及对应中断服务函数
 * 2, 支持对OV7725 VSYNC中断的处理
 * 3, 增加 g_ov7725_vsta 全局变量
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/BEEP/beep.h"
#include "./BSP/KEY/key.h"
#include "./BSP/EXTI/exti.h"
#include "./BSP/OV7725/ov7725.h"
#include "ucos_ii.h"

/**
 * @brief       KEY0 外部中断服务程序
 * @param       无
 * @retval      无
 */
void KEY0_INT_IRQHandler(void)
{
    delay_ms(20);  /* 消抖 */
    EXTI->PR = KEY0_INT_GPIO_PIN;   /* 清除KEY0所在中断线 的中断标志位 */

    if (KEY0 == 0)
    {
        LED0_TOGGLE();  /* LED0 状态取反 */
        LED1_TOGGLE();  /* LED1 状态取反 */
    }
}

/**
 * @brief       KEY1 外部中断服务程序
 * @param       无
 * @retval      无
 */
void KEY1_INT_IRQHandler(void)
{
    delay_ms(20);  /* 消抖 */
    EXTI->PR = KEY1_INT_GPIO_PIN;   /* 清除KEY1所在中断线 的中断标志位 */

    if (KEY1 == 0)
    {
        LED0_TOGGLE();  /* LED0 状态取反 */
    }
}

/**
 * @brief       KEY1 外部中断服务程序
 * @param       无
 * @retval      无
 */
void KEY2_INT_IRQHandler(void)
{
    delay_ms(20);  /* 消抖 */
    EXTI->PR = KEY2_INT_GPIO_PIN;   /* 清除KEY2所在中断线 的中断标志位 */

    if (KEY2 == 0)
    {
        LED1_TOGGLE();  /* LED1 状态取反 */
    }
}

/**
 * @brief       WK_UP 外部中断服务程序
 * @param       无
 * @retval      无
 */
void WKUP_INT_IRQHandler(void)
{
    delay_ms(20);  /* 消抖 */
    EXTI->PR = WKUP_INT_GPIO_PIN;   /* 清除WKUP所在中断线 的中断标志位 */

    if (WK_UP == 1)
    {
        BEEP_TOGGLE();  /* LED2 状态取反 */
    }
}

/**
 * @brief       外部中断初始化程序
 * @param       无
 * @retval      无
 */
void extix_init(void)
{
    key_init();
    sys_nvic_ex_config(KEY0_INT_GPIO_PORT, KEY0_INT_GPIO_PIN, SYS_GPIO_FTIR);   /* KEY0配置为下降沿触发中断 */
    sys_nvic_ex_config(KEY1_INT_GPIO_PORT, KEY1_INT_GPIO_PIN, SYS_GPIO_FTIR);   /* KEY1配置为下降沿触发中断 */
    sys_nvic_ex_config(KEY2_INT_GPIO_PORT, KEY2_INT_GPIO_PIN, SYS_GPIO_FTIR);   /* KEY2配置为下降沿触发中断 */
    sys_nvic_ex_config(WKUP_INT_GPIO_PORT, WKUP_INT_GPIO_PIN, SYS_GPIO_RTIR);   /* WKUP配置为上升沿触发中断 */

    sys_nvic_init( 0, 2, KEY0_INT_IRQn, 2); /* 抢占0，子优先级2，组2 */
    sys_nvic_init( 1, 2, KEY1_INT_IRQn, 2); /* 抢占1，子优先级2，组2 */
    sys_nvic_init( 2, 2, KEY2_INT_IRQn, 2); /* 抢占2，子优先级2，组2 */
    sys_nvic_init( 3, 2, WKUP_INT_IRQn, 2); /* 抢占3，子优先级2，组2 */
}

/* OV7725 帧中断标志
 * 0, 表示上一帧数据已经处理完了, 新的帧中断到来, 可以往FIFO写数据
 * 1, 表示上一帧数据还没有处理完, 新的帧中断到来, 不能往FIFO写数据
 */
uint8_t g_ov7725_vsta = 0;

/**
 * @brief       OV7725 VSYNC 外部中断服务程序
 * @param       无
 * @retval      无
 */
void OV7725_VSYNC_INT_IRQHandler(void)
{
    OSIntEnter();
    
    if (EXTI->PR & OV7725_VSYNC_GPIO_PIN)   /* 是OV7725_VSYNC_GPIO_PIN线的中断 */
    {
        if (g_ov7725_vsta == 0) /* 上一帧数据已经处理了? */
        {
            OV7725_WRST(0);     /* 复位写指针 */
            OV7725_WRST(1);     /* 结束复位 */
            OV7725_WEN(1);      /* 允许写入FIFO */
            g_ov7725_vsta = 1;  /* 标记帧中断 */
        }
        else
        {
            OV7725_WEN(0);      /* 禁止写入FIFO */
        }
    }

    EXTI->PR = OV7725_VSYNC_GPIO_PIN;   /* 清除OV7725_VSYNC_GPIO_PIN上的中断标志位 */

    OSIntExit();
}

/**
 * @brief       OV7725 VSYNC外部中断初始化程序
 * @param       无
 * @retval      无
 */
void exti_ov7725_vsync_init(void)
{
    sys_nvic_ex_config(OV7725_VSYNC_GPIO_PORT, OV7725_VSYNC_GPIO_PIN, SYS_GPIO_RTIR);   /* OV7725 VSYNC脚 上升沿触发中断 */
    sys_nvic_init( 0, 0, OV7725_VSYNC_INT_IRQn, 2);                                     /* 抢占0，子优先级0，组2 */
}











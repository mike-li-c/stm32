/**
 ****************************************************************************************************
 * @file        can.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-24
 * @brief       CAN 驱动代码
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

#ifndef __CAN_H
#define __CAN_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* CAN 引脚 定义 */

#define CAN_RX_GPIO_PORT                GPIOA
#define CAN_RX_GPIO_PIN                 SYS_GPIO_PIN11
#define CAN_RX_GPIO_CLK_ENABLE()        do{ RCC->APB2ENR |= 1 << 2; }while(0)   /* PA口时钟使能 */

#define CAN_TX_GPIO_PORT                GPIOA
#define CAN_TX_GPIO_PIN                 SYS_GPIO_PIN12
#define CAN_TX_GPIO_CLK_ENABLE()        do{ RCC->APB2ENR |= 1 << 2; }while(0)   /* PA口时钟使能 */

/******************************************************************************************/

/* CAN接收RX0中断使能 */
#define CAN_RX0_INT_ENABLE      0               /* 0,不使能; 1,使能; */

/* 静态函数, 仅can.c可以调用 */
static uint8_t can_pend_msg(uint8_t fifox);     /* CAN 查询数据 */
static uint8_t can_tx_status(uint8_t mbox);     /* CAN 查询发送状态 */
static uint8_t can_tx_msg(uint32_t id, uint8_t ide, uint8_t rtr, uint8_t len, uint8_t *data);
static void can_rx_msg(uint8_t fifox, uint32_t *id, uint8_t *ide, uint8_t *rtr, uint8_t *len, uint8_t *data);

/* 对外接口函数 */
uint8_t can_receive_msg(uint32_t id, uint8_t *buf);             /* CAN接收数据, 查询 */
uint8_t can_send_msg(uint32_t id, uint8_t *msg, uint8_t len);   /* CAN发送数据 */
uint8_t can_init(uint8_t tsjw,uint8_t tbs2,uint8_t tbs1,uint16_t brp,uint8_t mode); /* CAN初始化 */

#endif


















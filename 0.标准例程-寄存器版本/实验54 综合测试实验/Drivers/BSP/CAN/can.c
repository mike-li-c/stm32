/**
 ****************************************************************************************************
 * @file        can.c
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

#include "./BSP/CAN/can.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"


/**
 * @brief       CAN初始化
 * @param       tsjw    : 重新同步跳跃时间单元.范围: 1~3;
 * @param       tbs2    : 时间段2的时间单元.范围: 1~8;
 * @param       tbs1    : 时间段1的时间单元.范围: 1~16;
 * @param       brp     : 波特率分频器.范围: 1~1024;
 *   @note      以上4个参数, 在函数内部会减1, 所以, 任何一个参数都不能等于0
 *              CAN挂在APB1上面, 其输入时钟频率为 Fpclk1 = PCLK1 = 36Mhz
 *              tq     = brp * tpclk1;
 *              波特率 = Fpclk1 / ((tbs1 + tbs2 + 1) * brp);
 *              我们设置 can_init(1, 8, 9, 4, 1), 则CAN波特率为:
 *              36M / ((8 + 9 + 1) * 4) = 500Kbps
 *
 * @param       mode    : 普通模式;1,回环模式;
 * @retval      0,  初始化成功; 其他, 初始化失败;
 */
uint8_t can_init(uint8_t tsjw, uint8_t tbs2, uint8_t tbs1, uint16_t brp, uint8_t mode)
{
    uint16_t i = 0;

    if (tsjw == 0 || tbs2 == 0 || tbs1 == 0 || brp == 0)
    {
        return 1;   /* 参数不合法, 直接返回 错误 */
    }
    
    tsjw -= 1;      /* 先减去1.再用于设置 */
    tbs2 -= 1;
    tbs1 -= 1;
    brp -= 1;
    
    CAN_RX_GPIO_CLK_ENABLE();   /* CAN_RX脚时钟使能 */
    CAN_TX_GPIO_CLK_ENABLE();   /* CAN_TX脚时钟使能 */
    
    sys_gpio_set(CAN_TX_GPIO_PORT, CAN_TX_GPIO_PIN,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* CAN_TX脚 模式设置 */

    sys_gpio_set(CAN_RX_GPIO_PORT, CAN_RX_GPIO_PIN,
                 SYS_GPIO_MODE_IN, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_PU);   /* CAN_RX脚 必须设置成输入模式 */

    RCC->APB1ENR |= 1 << 25;    /* 使能CAN时钟 CAN使用的是APB1的时钟(max: 36M) */
    CAN1->MCR = 0x0000;         /* 退出睡眠模式(同时设置所有位为0) */
    CAN1->MCR |= 1 << 0;        /* 请求CAN进入初始化模式 */

    while ((CAN1->MSR & 1 << 0) == 0)   /* 等待进入初始化模式成功 */
    {
        i++;

        if (i > 100)            /* 进入初始化模式失败 */
        {
            return 2;
        }
    }

    CAN1->MCR |= 0 << 7;        /* 非时间触发通信模式 */
    CAN1->MCR |= 0 << 6;        /* 软件自动离线管理 */
    CAN1->MCR |= 0 << 5;        /* 睡眠模式通过软件唤醒(清除CAN1->MCR的SLEEP位) */
    CAN1->MCR |= 1 << 4;        /* 禁止报文自动传送 */
    CAN1->MCR |= 0 << 3;        /* 报文不锁定,新的覆盖旧的 */
    CAN1->MCR |= 0 << 2;        /* 优先级由报文标识符决定 */
    CAN1->BTR = 0;              /* 清除原来的设置. */
    CAN1->BTR |= mode << 30;    /* LBKM = mode, 模式设置  0, 普通模式; 1, 回环模式; */
    CAN1->BTR |= tsjw << 24;    /* SJW[1:0] = tsjw, 重新同步跳跃宽度(Tsjw)为 tsjw + 1 个时间单位 */
    CAN1->BTR |= tbs2 << 20;    /* TS2[2:0] = tbs2, Tbs2 = tbs2 + 1 个时间单位 */
    CAN1->BTR |= tbs1 << 16;    /* TS1[2:0] = tbs1, Tbs1 = tbs1 + 1 个时间单位 */
    CAN1->BTR |= brp << 0;      /* BRP[9:0] = brp, Fdiv = brp + 1 */
    
    /* 波特率: Fpclk1 / ((Tbs1 + Tbs2 + 1) * Fdiv) */
    CAN1->MCR &= ~(1 << 0);     /* 请求CAN退出初始化模式 */

    while ((CAN1->MSR & 1 << 0) == 1)
    {
        i++;

        if (i > 0XFFF0)
        {
            return 3;           /* 退出初始化模式失败 */
        }
    }

    /* 过滤器初始化, 这里只用到了过滤器0, 且不过滤任何ID */
    CAN1->FMR |= 1 << 0;        /* 过滤器组工作在初始化模式 */
    CAN1->FA1R &= ~(1 << 0);    /* 过滤器0不激活 */
    CAN1->FS1R |= 1 << 0;       /* 过滤器位宽为32位. */
    CAN1->FM1R |= 0 << 0;       /* 过滤器0工作在标识符屏蔽位模式 */
    CAN1->FFA1R |= 0 << 0;      /* 过滤器0关联到FIFO0 */
    CAN1->sFilterRegister[0].FR1 = 0X00000000;  /* 32位ID */
    CAN1->sFilterRegister[0].FR2 = 0X00000000;  /* 32位MASK */
    CAN1->FA1R |= 1 << 0;       /* 激活过滤器0 */
    CAN1->FMR &= 0 << 0;        /* 过滤器组进入正常模式 */

#if CAN_RX0_INT_ENABLE

    /* 使用中断接收 */
    CAN1->IER |= 1 << 1;        /* FIFO0消息挂号中断允许 */
    sys_nvic_init(1, 0, USB_LP_CAN1_RX0_IRQn, 2);   /* 组2 */

#endif
    return 0;
}

/**
 * @brief       CAN发送一条消息
 * @param       id      : 标准ID(11位)/扩展ID(11位+18位)
 * @param       ide     : 0,标准帧;1,扩展帧
 * @param       rtr     : 0,数据帧;1,远程帧
 * @param       len     : 要发送的数据长度(固定为8个字节, 在时间触发模式下, 有效数据为6个字节)
 * @param       data    : 数据指针
 * @retval      本次发送数据所用的邮箱号
 *   @arg       0 ~ 3   : 邮箱编号
 *   @arg       0XFF    : 无有效邮箱(数据发送失败)
 */
static uint8_t can_tx_msg(uint32_t id, uint8_t ide, uint8_t rtr, uint8_t len, uint8_t *data)
{
    uint8_t mbox;

    if (CAN1->TSR & (1 << 26))      /* 邮箱0为空, mbox = 0 */
    {
        mbox = 0;
    }
    else if (CAN1->TSR & (1 << 27)) /* 邮箱1为空, mbox = 1 */
    {
        mbox = 1;
    }
    else if (CAN1->TSR & (1 << 28)) /* 邮箱2为空, mbox = 2 */
    {
        mbox = 2;
    }
    else
    {
        return 0XFF;                /* 无空邮箱, 无法发送 */
    }
    
    CAN1->sTxMailBox[mbox].TIR = 0; /* 清除之前的设置 */

    if (ide == 0)           /* 标准帧 */
    {
        id &= 0x7ff;        /* 取低11位stdid */
        id <<= 21;
    }
    else                    /* 扩展帧 */
    {
        id &= 0X1FFFFFFF;   /* 取低32位extid */
        id <<= 3;
    }

    CAN1->sTxMailBox[mbox].TIR |= id;
    CAN1->sTxMailBox[mbox].TIR |= ide << 2;
    CAN1->sTxMailBox[mbox].TIR |= rtr << 1;
    len &= 0X0F;            /* 得到低四位, 最长8个字节 */
    CAN1->sTxMailBox[mbox].TDTR &= ~(0XF << 0);
    CAN1->sTxMailBox[mbox].TDTR |= len; /* 设置DLC[3:0] */
    
    /* 待发送数据存入邮箱 */
    CAN1->sTxMailBox[mbox].TDHR = (((uint32_t)data[7] << 24) |
                                   ((uint32_t)data[6] << 16) |
                                   ((uint32_t)data[5] << 8) |
                                   ((uint32_t)data[4]));

    CAN1->sTxMailBox[mbox].TDLR = (((uint32_t)data[3] << 24) |
                                   ((uint32_t)data[2] << 16) |
                                   ((uint32_t)data[1] << 8) |
                                   ((uint32_t)data[0]));

    CAN1->sTxMailBox[mbox].TIR |= 1 << 0;   /* 请求发送邮箱数据 */
    return mbox;
}

/**
 * @brief       获取CAN发送状态
 * @param       mbox    : 邮箱编号 (0 ~ 2)
 * @retval      发送状态
 *   @arg       0X00    : 挂起
 *   @arg       0X05    : 发送失败
 *   @arg       0X07    : 发送成功
 */
static uint8_t can_tx_status(uint8_t mbox)
{
    uint8_t sta = 0;

    switch (mbox)
    {
        case 0:
            sta |= CAN1->TSR & (1 << 0);            /* RQCP0 */
            sta |= CAN1->TSR & (1 << 1);            /* TXOK0 */
            sta |= ((CAN1->TSR & (1 << 26)) >> 24); /* TME0 */
            break;

        case 1:
            sta |= CAN1->TSR & (1 << 8) >> 8;       /* RQCP1 */
            sta |= CAN1->TSR & (1 << 9) >> 8;       /* TXOK1 */
            sta |= ((CAN1->TSR & (1 << 27)) >> 25); /* TME1 */
            break;

        case 2:
            sta |= CAN1->TSR & (1 << 16) >> 16;     /* RQCP2 */
            sta |= CAN1->TSR & (1 << 17) >> 16;     /* TXOK2 */
            sta |= ((CAN1->TSR & (1 << 28)) >> 26); /* TME2 */
            break;

        default:
            sta = 0X05; /* 邮箱号不对,肯定失败 */
            break;
    }

    return sta;
}

/**
 * @brief       得到在FIFO0/FIFO1中接收到的报文个数
 * @param       fifox   : 0, FIFO0; 1, FIFO1;
 * @retval      FIFO0 / FIFO1中报文的个数
 */
uint8_t can_pend_msg(uint8_t fifox)
{
    if (fifox == 0)
    {
        return CAN1->RF0R & 0x03;   /* 返回FIFO0中报文个数 */
    }
    else
    {
        return CAN1->RF1R & 0x03;   /* 返回FIFO1中报文个数 */
    }
}

/**
 * @brief       CAN接收一条消息
 * @param       fifox   : FIFO编号 0, FIFO0; 1, FIFO1;
 * @param       id      : 标准ID(11位)/扩展ID(11位+18位)
 * @param       ide     : 0,标准帧;1,扩展帧
 * @param       rtr     : 0,数据帧;1,远程帧
 * @param       len     : 接收到的数据长度(一般为8个字节)
 * @param       data    : 数据缓冲区
 * @retval      无
 */
static void can_rx_msg(uint8_t fifox, uint32_t *id, uint8_t *ide, uint8_t *rtr, uint8_t *len, uint8_t *data)
{
    *ide = CAN1->sFIFOMailBox[fifox].RIR & 0x04;    /* 得到标识符选择位的值 */

    if (*ide == 0)  /* 标准标识符 */
    {
        *id = CAN1->sFIFOMailBox[fifox].RIR >> 21;
    }
    else            /* 扩展标识符 */
    {
        *id = CAN1->sFIFOMailBox[fifox].RIR >> 3;
    }

    *rtr = CAN1->sFIFOMailBox[fifox].RIR & 0x02;    /* 得到远程发送请求值 */
    *len = CAN1->sFIFOMailBox[fifox].RDTR & 0x0F;   /* 得到DLC[3:0] */

    /* 接收数据 */
    data[0] = CAN1->sFIFOMailBox[fifox].RDLR & 0XFF;
    data[1] = (CAN1->sFIFOMailBox[fifox].RDLR >> 8) & 0XFF;
    data[2] = (CAN1->sFIFOMailBox[fifox].RDLR >> 16) & 0XFF;
    data[3] = (CAN1->sFIFOMailBox[fifox].RDLR >> 24) & 0XFF;
    data[4] = CAN1->sFIFOMailBox[fifox].RDHR & 0XFF;
    data[5] = (CAN1->sFIFOMailBox[fifox].RDHR >> 8) & 0XFF;
    data[6] = (CAN1->sFIFOMailBox[fifox].RDHR >> 16) & 0XFF;
    data[7] = (CAN1->sFIFOMailBox[fifox].RDHR >> 24) & 0XFF;

    if (fifox == 0)
    {
        CAN1->RF0R |= 0X20; /* 释放FIFO0邮箱 */
    }
    else
    {
        CAN1->RF1R |= 0X20; /* 释放FIFO1邮箱 */
    }
}

#if CAN_RX0_INT_ENABLE      /* 使能RX0中断 */

/**
 * @brief       CAN RX0 中断服务函数
 *   @note      处理CAN FIFO0的接收中断
 * @param       无
 * @retval      无
 */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    uint8_t rxbuf[8];
    uint32_t id;
    uint8_t ide, rtr, len;
    can_rx_msg(0, &id, &ide, &rtr, &len, rxbuf);
    printf("id:%d\r\n", id);
    printf("ide:%d\r\n", ide);
    printf("rtr:%d\r\n", rtr);
    printf("len:%d\r\n", len);
    printf("rxbuf[0]:%d\r\n", rxbuf[0]);
    printf("rxbuf[1]:%d\r\n", rxbuf[1]);
    printf("rxbuf[2]:%d\r\n", rxbuf[2]);
    printf("rxbuf[3]:%d\r\n", rxbuf[3]);
    printf("rxbuf[4]:%d\r\n", rxbuf[4]);
    printf("rxbuf[5]:%d\r\n", rxbuf[5]);
    printf("rxbuf[6]:%d\r\n", rxbuf[6]);
    printf("rxbuf[7]:%d\r\n", rxbuf[7]);
}

#endif

/**
 * @brief       CAN 发送一组数据
 *   @note      发送格式固定为: 标准ID, 数据帧
 * @param       id      : 标准ID(11位)
 * @retval      发送状态 0, 成功; 1, 失败;
 */
uint8_t can_send_msg(uint32_t id, uint8_t *msg, uint8_t len)
{
    uint8_t mbox;
    uint16_t t = 0;
    uint8_t rval = 1;   /* 默认标记发送失败 */
    mbox = can_tx_msg(id, 0, 0, len, msg);  /* 发送消息 */

    while (t < 0XFFF)
    {
        if(can_tx_status(mbox) == 0X07)     /* 发送完成 */
        {
            rval = 0;   /* 标记成功 */
            break;
        }

        t++;
    }

    return rval;        /* 返回发送结果 */
}

/**
 * @brief       CAN 接收数据查询
 *   @note      接收数据格式固定为: 标准ID, 数据帧
 * @param       id      : 要查询的 标准ID(11位)
 * @param       buf     : 数据缓存区
 * @retval      接收结果
 *   @arg       0   , 无数据被接收到;
 *   @arg       其他, 接收的数据长度
 */
uint8_t can_receive_msg(uint32_t id, uint8_t *buf)
{
    uint32_t rid = 0;
    uint8_t ide, rtr, len = 0;

    if (can_pend_msg(0) == 0)return 0;          /* 没有接收到数据,直接退出 */

    can_rx_msg(0, &rid, &ide, &rtr, &len, buf); /* 读取数据 */

    if (rid!= id || ide != 0 || rtr != 0)       /* 接收到的ID不对 / 不是标准帧 / 不是数据帧 */
    {
        len = 0;    /* 接收错误 */
    }

    return len;
}















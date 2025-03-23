/**
 ****************************************************************************************************
 * @file        usbd_conf.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-05-13
 * @brief       usbd_conf 驱动代码
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
 * V1.0 20200513
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_msc.h"
#include "stm32f1xx_hal_pcd.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "ucos_ii.h"


/* PCD定义 */
PCD_HandleTypeDef hpcd;


/* USB连接状态
 * 0,没有连接;
 * 1,已经连接;
 */
volatile uint8_t g_device_state = 0;    /* 默认没有连接 */

/**
 * @brief       USB接口配置
 *   @note      使能/关闭USB接口, 以便每次启动都可以正常
 *              连接USB
 * @param       state   : 接口状态
 *   @arg       0, 断开USB连接
 *   @arg       1, 使能USB连接
 * @retval      无
 */
void usbd_port_config(uint8_t state)
{
    RCC->APB2ENR |= 1 << 2;     /* 使能PORTA时钟 */
    if (state)
    {
        USB->CNTR &= ~(1 << 1); /* PWDN = 0, 退出断电模式 */

        sys_gpio_set(GPIOA, SYS_GPIO_PIN11,
                     SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_NONE);     /* PA11引脚模式设置,复用功能 */

        sys_gpio_set(GPIOA, SYS_GPIO_PIN12,
                     SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_NONE);     /* PA12引脚模式设置,复用功能 */
    }
    else
    {
        USB->CNTR |= 1 << 1;    /* PWDN = 1, 进入断电模式 */

        sys_gpio_set(GPIOA, SYS_GPIO_PIN11,
                     SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_NONE);    /* PA11引脚模式设置,推挽输出 */

        sys_gpio_set(GPIOA, SYS_GPIO_PIN12,
                     SYS_GPIO_MODE_OUT, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_NONE);    /* PA12引脚模式设置,推挽输出 */

        sys_gpio_pin_set(GPIOA, SYS_GPIO_PIN11, 0); /* PA11 = 0 */
        sys_gpio_pin_set(GPIOA, SYS_GPIO_PIN12, 0); /* PA12 = 0 */
    }
}

/**
 * @brief       初始化PCD MSP
 *   @note      这是一个回调函数, 在stm32f1xx_hal_pcd.c里面调用
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    RCC->APB2ENR |= 1 << 2;             /* 使能PORTA时钟 */
    RCC->APB1ENR |= 1 << 23;            /* 使能USB时钟 */

    sys_gpio_set(GPIOA, SYS_GPIO_PIN11,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_NONE); /* PA11引脚模式设置,复用功能 */

    sys_gpio_set(GPIOA, SYS_GPIO_PIN12,
                 SYS_GPIO_MODE_AF, SYS_GPIO_OTYPE_PP, SYS_GPIO_SPEED_HIGH, SYS_GPIO_PUPD_NONE); /* PA12引脚模式设置,复用功能 */

    sys_nvic_init(0, 3, USB_LP_CAN1_RX0_IRQn, 2);   /* 优先级设置为抢占0,子优先级3，组2 */
}

/**
 * @brief       USB OTG 中断服务函数
 *   @note      处理所有USB中断
 * @param       无
 * @retval      无
 */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    OSIntEnter();
    HAL_PCD_IRQHandler(&hpcd);
    OSIntExit();
}

/******************************************************************************************/
/* 以下是: USBD LL PCD 驱动的回调函数(PCD->USB Device Library) */


/**
 * @brief       USBD 配置阶段回调函数
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage(hpcd->pData, (uint8_t *) hpcd->Setup);
}

/**
 * @brief       USBD OUT 阶段回调函数
 * @param       hpcd    : PCD结构体指针
 * @param       epnum   : 端点号
 * @retval      无
 */
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage(hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

/**
 * @brief       USBD IN 阶段回调函数
 * @param       hpcd    : PCD结构体指针
 * @param       epnum   : 端点号
 * @retval      无
 */
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage(hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

/**
 * @brief       USBD SOF 回调函数
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF(hpcd->pData);
}

/**
 * @brief       USBD 复位回调函数
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    printf("USB Device Library  [FS]\r\n");
    USBD_LL_SetSpeed((USBD_HandleTypeDef *) hpcd->pData, USBD_SPEED_FULL);
    USBD_LL_Reset((USBD_HandleTypeDef *) hpcd->pData);  /* 复位设备 */
}

/**
 * @brief       USBD 挂起回调函数
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
    g_device_state = 0;
    printf("Device In suspend mode.\r\n");
    USBD_LL_Suspend(hpcd->pData);
}

/**
 * @brief       USBD 恢复回调函数
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
    printf("Device Resumed\r\n");
    USBD_LL_Resume(hpcd->pData);
}

/**
 * @brief       USBD ISO OUT 事务完成回调函数
 * @param       hpcd    : PCD结构体指针
 * @param       epnum   : 端点号
 * @retval      无
 */
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete(hpcd->pData, epnum);
}

/**
 * @brief       USBD ISO IN 事务完成回调函数
 * @param       hpcd    : PCD结构体指针
 * @param       epnum   : 端点号
 * @retval      无
 */
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete(hpcd->pData, epnum);
}

/**
 * @brief       USBD 连接成功回调函数
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    g_device_state = 1;
    USBD_LL_DevConnected(hpcd->pData);
}

/**
 * @brief       USBD 断开连接回调函数
 * @param       hpcd    : PCD结构体指针
 * @retval      无
 */
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    g_device_state = 0;
    printf("USB Device Disconnected.\r\n");
    USBD_LL_DevDisconnected(hpcd->pData);
}

/******************************************************************************************/
/* 以下是: USBD LL 驱动接口函数(PCD->USB Device Library) */


/**
 * @brief       USBD 底层初始化函数
 * @param       pdev    : USBD句柄指针
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    /* 设置LL驱动相关参数 */
    hpcd.Instance = USB;                    /* 使用USB */
    hpcd.Init.dev_endpoints = 8;            /* 端点数为8 */
    hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;/* 使用内部PHY */
    hpcd.Init.speed = PCD_SPEED_FULL;       /* USB全速(12Mbps) */
    hpcd.Init.low_power_enable = 0;         /* 不使能低功耗模式 */

    hpcd.pData = pdev;                      /* hpcd的pData指向pdev */
    pdev->pData = &hpcd;                    /* pdev的pData指向hpcd */

    HAL_PCD_Init((PCD_HandleTypeDef *) pdev->pData);    /* 初始化LL驱动 */

    HAL_PCDEx_PMAConfig(pdev->pData, 0x00, PCD_SNG_BUF, 0x18);
    HAL_PCDEx_PMAConfig(pdev->pData, 0x80, PCD_SNG_BUF, 0x58);
    HAL_PCDEx_PMAConfig(pdev->pData, MSC_EPIN_ADDR, PCD_SNG_BUF, 0x98);
    HAL_PCDEx_PMAConfig(pdev->pData, MSC_EPOUT_ADDR, PCD_SNG_BUF, 0xD8);

    return USBD_OK;
}

/**
 * @brief       USBD 底层取消初始化(回复默认复位状态)函数
 * @param       pdev    : USBD句柄指针
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    HAL_PCD_DeInit(pdev->pData);
    return USBD_OK;
}

/**
 * @brief       USBD 底层驱动开始工作
 * @param       pdev    : USBD句柄指针
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    HAL_PCD_Start(pdev->pData);
    return USBD_OK;
}

/**
 * @brief       USBD 底层驱动停止工作
 * @param       pdev    : USBD句柄指针
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    HAL_PCD_Stop(pdev->pData);
    return USBD_OK;
}

/**
 * @brief       USBD 初始化(打开)某个端点
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @param       ep_type : 端点类型
 * @param       ep_mps  : 端点最大包容量(字节)
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps)
{
    HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type);
    return USBD_OK;
}

/**
 * @brief       USBD 取消初始化(关闭)某个端点
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_PCD_EP_Close(pdev->pData, ep_addr);
    return USBD_OK;
}

/**
 * @brief       USBD 清空某个端点的数据
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_PCD_EP_Flush(pdev->pData, ep_addr);
    return USBD_OK;
}

/**
 * @brief       USBD 给某个端点设置一个暂停状态
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_PCD_EP_SetStall(pdev->pData, ep_addr);
    return USBD_OK;
}

/**
 * @brief       USBD 取消某个端点的暂停状态
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev,
                                        uint8_t ep_addr)
{
    HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);
    return USBD_OK;
}

/**
 * @brief       USBD 返回是否处于暂停状态
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @retval      0, 非暂停; 1, 暂停;
 */
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = pdev->pData;

    if ((ep_addr & 0x80) == 0x80)
    {
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
    }
    else
    {
        return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
    }
}

/**
 * @brief       USBD 为设备指定新的USB地址
 * @param       pdev    : USBD句柄指针
 * @param       dev_addr: 新的设备地址,USB1_OTG_HS/USB2_OTG_HS
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    g_device_state = 1; /* 能执行到该函数,说明USB连接成功了 */
    HAL_PCD_SetAddress(pdev->pData, dev_addr);
    return USBD_OK;
}

/**
 * @brief       USBD 通过端点发送数据
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @param       pbuf    : 数据缓冲区首地址
 * @param       size    : 要发送的数据大小
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size)
{
    HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size);
    return USBD_OK;
}

/**
 * @brief       USBD 准备一个端点接收数据
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @param       pbuf    : 数据缓冲区首地址
 * @param       size    : 要接收的数据大小
 * @retval      USB状态
 *   @arg       USBD_OK(0)   , 正常;
 *   @arg       USBD_BUSY(1) , 忙;
 *   @arg       USBD_FAIL(2) , 失败;
 */
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size)
{
    HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size);
    return USBD_OK;
}

/**
 * @brief       USBD 获取最后一个传输包的大小
 * @param       pdev    : USBD句柄指针
 * @param       ep_addr : 端点号
 * @retval      包大小
 */
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return HAL_PCD_EP_GetRxCount(pdev->pData, ep_addr);
}

/**
 * @brief       USBD 延时函数(以ms为单位)
 * @param       Delay   : 延时的ms数
 * @retval      无
 */
void USBD_LL_Delay(uint32_t Delay)
{
    delay_ms(Delay);
}




























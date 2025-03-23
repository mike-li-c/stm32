/**
 ****************************************************************************************************
 * @file        spi.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.1
 * @date        2020-04-29
 * @brief       SPI ��������
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� STM32F103������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20200424
 * ��һ�η���
 * V1.1 20200429
 * 1, ���SPI1֧��
 * 2, ���spi1_init/spi1_set_speed��spi1_read_write_byte��������
 ****************************************************************************************************
 */

#ifndef __SPI_H
#define __SPI_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* SPI2 ���� ���� */

#define SPI2_SCK_GPIO_PORT              GPIOB
#define SPI2_SCK_GPIO_PIN               SYS_GPIO_PIN13
#define SPI2_SCK_GPIO_CLK_ENABLE()      do{ RCC->APB2ENR |= 1 << 3; }while(0)   /* PB��ʱ��ʹ�� */

#define SPI2_MISO_GPIO_PORT             GPIOB
#define SPI2_MISO_GPIO_PIN              SYS_GPIO_PIN14
#define SPI2_MISO_GPIO_CLK_ENABLE()     do{ RCC->APB2ENR |= 1 << 3; }while(0)   /* PB��ʱ��ʹ�� */

#define SPI2_MOSI_GPIO_PORT             GPIOB
#define SPI2_MOSI_GPIO_PIN              SYS_GPIO_PIN15
#define SPI2_MOSI_GPIO_CLK_ENABLE()     do{ RCC->APB2ENR |= 1 << 3; }while(0)   /* PB��ʱ��ʹ�� */

/* SPI2��ض��� */
#define SPI2_SPI                        SPI2
#define SPI2_SPI_CLK_ENABLE()           do{ RCC->APB1ENR |= 1 << 14; }while(0)  /* SPI2ʱ��ʹ�� */


/* SPI1 ���� ���� */

#define SPI1_SCK_GPIO_PORT              GPIOA
#define SPI1_SCK_GPIO_PIN               SYS_GPIO_PIN5
#define SPI1_SCK_GPIO_CLK_ENABLE()      do{ RCC->APB2ENR |= 1 << 2; }while(0)   /* PA��ʱ��ʹ�� */

#define SPI1_MISO_GPIO_PORT             GPIOA
#define SPI1_MISO_GPIO_PIN              SYS_GPIO_PIN6
#define SPI1_MISO_GPIO_CLK_ENABLE()     do{ RCC->APB2ENR |= 1 << 2; }while(0)   /* PA��ʱ��ʹ�� */

#define SPI1_MOSI_GPIO_PORT             GPIOA
#define SPI1_MOSI_GPIO_PIN              SYS_GPIO_PIN7
#define SPI1_MOSI_GPIO_CLK_ENABLE()     do{ RCC->APB2ENR |= 1 << 2; }while(0)   /* PA��ʱ��ʹ�� */

/* SPI2��ض��� */
#define SPI1_SPI                        SPI1
#define SPI1_SPI_CLK_ENABLE()           do{ RCC->APB2ENR |= 1 << 12; }while(0)  /* SPI1ʱ��ʹ�� */

/******************************************************************************************/


/* SPI�����ٶ����� */
#define SPI_SPEED_2         0
#define SPI_SPEED_4         1
#define SPI_SPEED_8         2
#define SPI_SPEED_16        3
#define SPI_SPEED_32        4
#define SPI_SPEED_64        5
#define SPI_SPEED_128       6
#define SPI_SPEED_256       7


void spi2_init(void);
void spi2_set_speed(uint8_t speed);
uint8_t spi2_read_write_byte(uint8_t txdata);

void spi1_init(void);
void spi1_set_speed(uint8_t speed);
uint8_t spi1_read_write_byte(uint8_t txdata);

#endif

























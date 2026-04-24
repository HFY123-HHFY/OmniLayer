#ifndef __103_HW_CONFIG_H
#define __103_HW_CONFIG_H

#include "LED.h"
#include "KEY.h"
#include "My_I2c.h"
#include "My_SPI.h"
#include "usart.h"
#include "pwm.h"
#include "adc.h"

/*
 * 103_hw_config.h 板级硬件映射宏
 */

/*
 * HW_LED_MAP(X) 的用途：
 * 1) X 是一个宏函数，用于把每个 LED 映射项展开成结构体初始化代码。
 * 2) 这样 Enroll.c 里只需要写一次模板，不用重复手写每个 LED 项。
 */
#define HW_LED_MAP(X) \
	X(LED1, GPIOC, GPIO_Pin_13)

/* USART1 引脚定义：TX=PA9，RX=PA10。 */
#define HW_USART1_TX_PORT GPIOA
#define HW_USART1_TX_PIN  GPIO_Pin_9
#define HW_USART1_RX_PORT GPIOA
#define HW_USART1_RX_PIN  GPIO_Pin_10

/* USART 板级映射：注册 1 路串口 */
#define HW_USART_MAP(X) \
	X(API_USART1, HW_USART1_TX_PORT, HW_USART1_TX_PIN, HW_USART1_RX_PORT, HW_USART1_RX_PIN)

/* 软件 I2C1 引脚定义：SCL=PB8，SDA=PB9 */
#define HW_I2C1_SCL_PORT GPIOB
#define HW_I2C1_SCL_PIN  GPIO_Pin_8
#define HW_I2C1_SDA_PORT GPIOB
#define HW_I2C1_SDA_PIN  GPIO_Pin_9

/* I2C 板级映射：注册 1 路软件 I2C */
#define HW_I2C_MAP(X) \
	X(My_I2C1, HW_I2C1_SCL_PORT, HW_I2C1_SCL_PIN, HW_I2C1_SDA_PIN)

/* 软件 SPI 引脚定义：CS=PA4，SCK=PA5，MOSI=PA7，MISO=PA6 */
#define HW_SPI1_CS_PORT   GPIOA
#define HW_SPI1_CS_PIN    GPIO_Pin_4
#define HW_SPI1_SCK_PORT  GPIOA
#define HW_SPI1_SCK_PIN   GPIO_Pin_5
#define HW_SPI1_MOSI_PORT GPIOA
#define HW_SPI1_MOSI_PIN  GPIO_Pin_7
#define HW_SPI1_MISO_PORT GPIOA
#define HW_SPI1_MISO_PIN  GPIO_Pin_6

/* SPI 板级映射：注册 1 路软件 SPI */
#define HW_SPI_MAP(X) \
	X(HW_SPI1_CS_PORT, HW_SPI1_CS_PIN, HW_SPI1_SCK_PIN, HW_SPI1_MOSI_PIN, HW_SPI1_MISO_PIN)

/* KEY 板级映射：注册 1 个按键 */
#define HW_KEY_MAP(X) \
	X(KEY1, GPIOB, GPIO_Pin_12)

/* PWM 板级映射 */
#define HW_PWM_MAP(X) \
	X(API_PWM_TIM2, API_PWM_CH1, GPIOA, GPIO_Pin_0) \
	X(API_PWM_TIM2, API_PWM_CH2, GPIOA, GPIO_Pin_1)

/* ADC 板级映射 */
#define HW_ADC_MAP(X) \
	X(API_ADC1, API_ADC_CH2, GPIOA, GPIO_Pin_1) \
	X(API_ADC1, API_ADC_CH3, GPIOA, GPIO_Pin_2)

/* MPU6050 INT 板级映射：仅维护引脚资源，优先级策略由 sys.c 统一管理 */
#define HW_MPU6050_INT_PORT             GPIOB
#define HW_MPU6050_INT_PIN              GPIO_Pin_5

/* 当前板子上注册了 1 个 LED */
#define HW_LED_COUNT  1U
/* 当前板子上注册了 1 路 USART */
#define HW_USART_COUNT  1U
/* 当前板子上注册了 1 路软件 I2C */
#define HW_I2C_COUNT  1U
/* 当前板子上注册了 1 路软件 SPI */
#define HW_SPI_COUNT  1U
/* 当前板子上注册了 1 个按键 */
#define HW_KEY_COUNT  1U
/* 当前板子上注册了 2 路 PWM 通道 */
#define HW_PWM_COUNT  2U
/* 当前板子上注册了 2 路 ADC 通道 */
#define HW_ADC_COUNT  2U

#endif /* __103_HW_CONFIG_H */

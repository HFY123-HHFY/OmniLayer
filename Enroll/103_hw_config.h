#ifndef __103_HW_CONFIG_H
#define __103_HW_CONFIG_H

#include "LED.h"
#include "KEY.h"
#include "My_I2c.h"
#include "My_SPI.h"
#include "usart.h"
#include "tim.h"
#include "pwm.h"
#include "adc.h"
#include "f103_gpio.h"

/*
 * 103_hw_config.h 板级硬件映射宏
 */

/*
 * HW_LED_MAP(X) 的用途：
 * 1) X 是一个宏函数，用于把每个 LED 映射项展开成结构体初始化代码。
 * 2) 这样 Enroll.c 里只需要写一次模板，不用重复手写每个 LED 项。
 */

/* LED 板级映射：注册 1 路LED */
#define HW_LED_MAP(X) \
	X(LED1, GPIOC, GPIO_Pin_13)

/* KEY 板级映射：注册 1 个按键 */
#define HW_KEY_MAP(X) \
	X(KEY1, GPIOB, GPIO_Pin_12)

/* USART1 引脚定义：TX=PA9，RX=PA10 */
#define HW_USART1_TX_PORT GPIOA
#define HW_USART1_TX_PIN  GPIO_Pin_9
#define HW_USART1_RX_PORT GPIOA
#define HW_USART1_RX_PIN  GPIO_Pin_10

/* USART 板级映射：注册 1 路串口 */
#define HW_USART_MAP(X) \
	X(API_USART1, API_USART_CORE_USART1, HW_USART1_TX_PORT, HW_USART1_TX_PIN, HW_USART1_RX_PORT, HW_USART1_RX_PIN)

/* TIM 板级映射：逻辑 API_TIM1 绑定到硬件 TIM3。 */
#define HW_TIM_MAP(X) \
	X(API_TIM1, API_TIM_CORE_TIM3)

/* PWM 板级映射 */
#define HW_PWM_MAP(X) \
	X(API_PWM_TIM2, API_PWM_CH1, API_PWM_CORE_TIM2, API_PWM_CORE_CH1, GPIOA, GPIO_Pin_0) \
	X(API_PWM_TIM2, API_PWM_CH2, API_PWM_CORE_TIM2, API_PWM_CORE_CH2, GPIOA, GPIO_Pin_1)

/* ADC 板级映射 */
#define HW_ADC_MAP(X) \
	X(API_ADC1, API_ADC_CH2, GPIOA, GPIO_Pin_1) \
	X(API_ADC1, API_ADC_CH3, GPIOA, GPIO_Pin_2)

/* 软件 I2C1 引脚定义：SCL=PB6，SDA=PB7 MPU6050 */
#define HW_I2C1_SCL_PORT GPIOB
#define HW_I2C1_SCL_PIN  GPIO_Pin_6
#define HW_I2C1_SDA_PORT GPIOB
#define HW_I2C1_SDA_PIN  GPIO_Pin_7

/* 软件 I2C2 引脚定义：SCL=PB8，SDA=PB9 OLED */
#define HW_I2C2_SCL_PORT GPIOB
#define HW_I2C2_SCL_PIN  GPIO_Pin_8
#define HW_I2C2_SDA_PORT GPIOB
#define HW_I2C2_SDA_PIN  GPIO_Pin_9

/* I2C 板级映射：注册 2 路软件 I2C */
#define HW_I2C_MAP(X) \
	X(My_I2C1, HW_I2C1_SCL_PORT, HW_I2C1_SCL_PIN, HW_I2C1_SDA_PIN) \
	X(My_I2C2, HW_I2C2_SCL_PORT, HW_I2C2_SCL_PIN, HW_I2C2_SDA_PIN)

/* 软件 SPI1 引脚定义：给 7 针 OLED */
#define HW_SPI1_SCK_PORT  GPIOA
#define HW_SPI1_SCK_PIN   GPIO_Pin_11

#define HW_SPI1_MOSI_PORT GPIOB
#define HW_SPI1_MOSI_PIN  GPIO_Pin_13

#define HW_SPI1_MISO_PORT GPIOB
#define HW_SPI1_MISO_PIN  GPIO_Pin_3

#define HW_SPI1_CS_PORT   GPIOA
#define HW_SPI1_CS_PIN    GPIO_Pin_8

/* 软件 SPI2 引脚定义：给 NRF24L01 */
#define HW_SPI2_CS_PORT   GPIOB
#define HW_SPI2_CS_PIN    GPIO_Pin_12

#define HW_SPI2_SCK_PORT  GPIOB
#define HW_SPI2_SCK_PIN   GPIO_Pin_13

#define HW_SPI2_MOSI_PORT GPIOB
#define HW_SPI2_MOSI_PIN  GPIO_Pin_15

#define HW_SPI2_MISO_PORT GPIOB
#define HW_SPI2_MISO_PIN  GPIO_Pin_14

/* SPI 板级映射：注册 2 路软件 SPI */
#define HW_SPI_MAP(X) \
	X(My_SPI1, HW_SPI1_CS_PORT, HW_SPI1_CS_PIN, \
	  HW_SPI1_SCK_PORT, HW_SPI1_SCK_PIN, \
	  HW_SPI1_MOSI_PORT, HW_SPI1_MOSI_PIN, \
	  HW_SPI1_MISO_PORT, HW_SPI1_MISO_PIN) \
	X(My_SPI2, HW_SPI2_CS_PORT, HW_SPI2_CS_PIN, \
	  HW_SPI2_SCK_PORT, HW_SPI2_SCK_PIN, \
	  HW_SPI2_MOSI_PORT, HW_SPI2_MOSI_PIN, \
	  HW_SPI2_MISO_PORT, HW_SPI2_MISO_PIN)

/* OLED SPI 控制引脚定义：DC=PB15，RES=PB14  */
#define HW_OLED_DC_PORT GPIOB
#define HW_OLED_DC_PIN  GPIO_Pin_15
#define HW_OLED_RES_PORT GPIOB
#define HW_OLED_RES_PIN  GPIO_Pin_14

/* OLED SPI 控制引脚映射：DC/RES */
#define HW_OLED_SPI_CTRL_MAP(X) \
	X(HW_OLED_DC_PORT, HW_OLED_DC_PIN, HW_OLED_RES_PORT, HW_OLED_RES_PIN)

/* NRF24L01 控制引脚定义：CE=PA12 */
#define HW_NRF24L01_CE_PORT GPIOA
#define HW_NRF24L01_CE_PIN  GPIO_Pin_12

/* NRF24L01 控制引脚映射：注册 1 组 CE */
#define HW_NRF24L01_CTRL_MAP(X) \
	X(HW_NRF24L01_CE_PORT, HW_NRF24L01_CE_PIN)

/* MPU6050 INT 板级映射：仅维护引脚资源，优先级策略由 sys.c 统一管理 */
#define HW_MPU6050_INT_PORT             GPIOB
#define HW_MPU6050_INT_PIN              GPIO_Pin_5

/* 当前板子上注册了 1 个 LED */
#define HW_LED_COUNT  1U

/* 当前板子上注册了 1 个按键 */
#define HW_KEY_COUNT   1U

/* 当前板子上注册了 1 路 USART */
#define HW_USART_COUNT  1U

/* 当前板子上注册了 1 路 TIM */
#define HW_TIM_COUNT  1U

/* 当前板子上注册了 2 路 PWM 通道 */
#define HW_PWM_COUNT  2U

/* 当前板子上注册了 2 路 ADC 通道 */
#define HW_ADC_COUNT  2U

/* 当前板子上注册了 2 路软件 I2C */
#define HW_I2C_COUNT  2U

/* 当前板子上注册了 2 路软件 SPI */
#define HW_SPI_COUNT  2U

/* 当前板子上注册了 1 组 OLED SPI 控制引脚 */
#define HW_OLED_SPI_CTRL_COUNT  1U
/* 当前板子上注册了 1 组 NRF24L01 控制引脚 */
#define HW_NRF24L01_CTRL_COUNT  1U

#endif /* __103_HW_CONFIG_H */

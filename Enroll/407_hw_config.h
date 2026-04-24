#ifndef __407_HW_CONFIG_H
#define __407_HW_CONFIG_H

#include "LED.h"
#include "KEY.h"
#include "My_I2c.h"
#include "usart.h"
#include "pwm.h"
#include "adc.h"

/*
 * 407_hw_config.h 板级硬件映射宏
 */

/*
 * HW_LED_MAP(X) 的用途：
 * 1) X 是一个宏函数，用于把每个 LED 映射项展开成结构体初始化代码。
 * 2) 这样 Enroll.c 里只需要写一次模板，不用重复手写每个 LED 项。
 */

/*
LED1 绿 LED2 红 LED3 蓝
*/
#define HW_LED_MAP(X) \
	X(LED1, GPIOE, GPIO_Pin_2) \
	X(LED2, GPIOE, GPIO_Pin_3) \
	X(LED3, GPIOE, GPIO_Pin_4)

/* USART 板级映射：当前板子注册 1 路串口*/
#define HW_USART_MAP(X) \
	X(API_USART1, GPIOA, GPIO_Pin_9, GPIOA, GPIO_Pin_10)

/* I2C 板级映射：My_I2C1 供传感器，My_I2C2 供 OLED。 */
#define HW_I2C_MAP(X) \
	X(My_I2C1, GPIOB, GPIO_Pin_8, GPIO_Pin_9) \
	X(My_I2C2, GPIOD, GPIO_Pin_5, GPIO_Pin_6)

/* KEY 板级映射：当前板子注册 1 个按键。 */
#define HW_KEY_MAP(X) \
	X(KEY1, GPIOA, GPIO_Pin_0)

/* PWM 板级映射：PE9 -> TIM1_CH1，PE11 -> TIM1_CH2，PE13 -> TIM1_CH3，PE14 -> TIM1_CH4 */
#define HW_PWM_MAP(X) \
	X(API_PWM_TIM1, API_PWM_CH1, GPIOE, GPIO_Pin_9) \
	X(API_PWM_TIM1, API_PWM_CH2, GPIOE, GPIO_Pin_11) \
	X(API_PWM_TIM1, API_PWM_CH3, GPIOE, GPIO_Pin_13) \
	X(API_PWM_TIM1, API_PWM_CH4, GPIOE, GPIO_Pin_14)

/* ADC 板级映射 */
#define HW_ADC_MAP(X) \
	X(API_ADC1, API_ADC_CH2, GPIOA, GPIO_Pin_2) \
	X(API_ADC1, API_ADC_CH3, GPIOA, GPIO_Pin_3)

/* MPU6050 INT 板级映射：仅维护引脚资源，优先级策略由 sys.c 统一管理。 */
#define HW_MPU6050_INT_PORT             GPIOE
#define HW_MPU6050_INT_PIN              GPIO_Pin_7

/* 当前板子上注册了 3 个 LED。 */
#define HW_LED_COUNT  3U
/* 当前板子上注册了 1 路 USART。 */
#define HW_USART_COUNT  1U
/* 当前板子上注册了 2 路软件 I2C。 */
#define HW_I2C_COUNT  2U
/* 当前板子上注册了 1 个按键。 */
#define HW_KEY_COUNT  1U
/* 当前板子上注册了 4 路 PWM 通道。 */
#define HW_PWM_COUNT  4U
/* 当前板子上注册了 2 路 ADC 通道。 */
#define HW_ADC_COUNT  2U

#endif /* __407_HW_CONFIG_H */

#ifndef __ENROLL_H
#define __ENROLL_H

#include <stdint.h>

/* MCU 目标常量：放在注册层头文件，便于统一查看与管理。 */
#ifndef ENROLL_MCU_F103
#define ENROLL_MCU_F103   0U
#endif

#ifndef ENROLL_MCU_F407
#define ENROLL_MCU_F407   1U
#endif

/*
 * 全局宏重定义 ENROLL_MCU_TARGET。
 */
#ifndef ENROLL_MCU_TARGET
#define ENROLL_MCU_TARGET  ENROLL_MCU_F407
#endif

#include "LED.h"
#include "KEY.h"
#include "My_I2c.h"
#include "gpio.h"
#include "usart.h"
#include "adc.h"

/*
 * 条件编译选择不同 MCU 的 hw_config。
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "103_hw_config.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "407_hw_config.h"
#else
#error "Unsupported ENROLL_MCU_TARGET. Use 0(F103) or 1(F407)."
#endif

#define ENROLL_GPIO_INIT_FN   API_GPIO_InitOutput
#define ENROLL_GPIO_INPUT_FN   API_GPIO_InitInputPullUp
#define ENROLL_GPIO_WRITE_FN  API_GPIO_Write
#define ENROLL_GPIO_READ_FN   API_GPIO_Read

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Enroll 层只负责“登记”和“对外转发接口”。
 * 这里的 LED 接口会把板级硬件映射和 Core 的 GPIO 驱动连接起来。
 */
void Enroll_LED_Init(LED_Level_t initLevel);

/* 把 app/main 的 LED 控制请求转发给 BSP。 */
void Enroll_LED_Control(LED_Id_t id, LED_Level_t level);

/* 先注册再初始化当前板子的 KEY 资源。 */
void Enroll_KEY_Init(void);

/* 注册当前板子的 USART 资源表。 */
void Enroll_USART_Register(void);

/* 注册当前板子的 I2C 资源表。 */
void Enroll_I2C_Register(void);

/* 注册当前板子的 PWM 引脚映射表。 */
void Enroll_PWM_Register(void);

/* 注册当前板子的 ADC 引脚映射表。 */
void Enroll_ADC_Register(void);

/* 根据板级映射注册 MPU6050 外部中断。 */
void Enroll_MPU6050_EXTI_Register(void);

#ifdef __cplusplus
}
#endif

#endif /* __ENROLL_LED_H */

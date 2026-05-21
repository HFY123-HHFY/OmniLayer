#ifndef __ENROLL_H
#define __ENROLL_H

#include <stdint.h>

/*
 * Enroll 层职责：
 * 1) 读取板级 hw_config 映射表；
 * 2) 把逻辑外设（LED/USART/ADC...）注册到 API/BSP；
 * 3) 对 App 暴露统一入口，避免 App 直接依赖具体 MCU 细节。
 */

/* MCU 目标常量：放在注册层头文件，便于统一查看与管理。 */
#ifndef ENROLL_MCU_F103
#define ENROLL_MCU_F103   0U
#endif

#ifndef ENROLL_MCU_F407
#define ENROLL_MCU_F407   1U
#endif

#ifndef ENROLL_MCU_G3507
#define ENROLL_MCU_G3507  2U
#endif

/* 若外部未定义目标 MCU，则默认使用 G3507。 */
#ifndef ENROLL_MCU_TARGET
#define ENROLL_MCU_TARGET  ENROLL_MCU_G3507
#endif

/*
 * 头文件依赖规则：
 * - Enroll.h 是对外接口头，凡是出现在函数声明中的类型，必须在这里可见。
 * - 因此以下头文件保留在 .h（不能简单下沉到 .c）。
 */
#include "LED.h"      /* LED_Id_t / LED_Level_t */
#include "KEY.h"      /* KEY 模块初始化接口 */
#include "gpio.h"     /* ENROLL_GPIO_xxx 宏映射到 API_GPIO_xxx */
#include "My_I2c.h"   /* 板级 I2C 映射宏可能使用 My_I2C 枚举 */
#include "pwm.h"      /* API_PWM_Tim_t */
#include "usart.h"    /* API_USART_Id_t / API_USART_IrqHandler_t */
#include "tim.h"      /* API_TIM_IrqHandler_t */
#include "adc.h"      /* API_ADC_Id_t */

/*
 * 条件编译选择不同 MCU 的 hw_config。
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "103_hw_config.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "407_hw_config.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#include "G3507_hw_config.h"
#else
#error "Unsupported ENROLL_MCU_TARGET. Use 0(F103), 1(F407), or 2(G3507)."
#endif

/* GPIO 统一经 API 层分发到对应 Core 实现。 */
#define ENROLL_GPIO_INIT_FN   API_GPIO_InitOutput
#define ENROLL_GPIO_INPUT_FN  API_GPIO_InitInputPullUp
#define ENROLL_GPIO_WRITE_FN  API_GPIO_Write
#define ENROLL_GPIO_READ_FN   API_GPIO_Read

#ifdef __cplusplus
extern "C" {
#endif

/* Enroll 层只负责资源登记，不负责初始化。 */
void Enroll_LED_Register(void);

/*
 * 设计说明：
 * - 真正控制 LED 的实现仍在 LED 模块（LED_Control）。
 * - Enroll_LED_Control 是门面转发，目的是让 App 仅依赖 Enroll，
 *   不直接耦合到底层 BSP 模块。
 */
void Enroll_LED_Control(LED_Id_t id, LED_Level_t level);

/* 串口资源注册：按板级映射绑定 API 与 Core。 */
void Enroll_USART_Register(void);
void Enroll_USART_RegisterIrqHandler(API_USART_IrqHandler_t handler);

/* 按键资源注册：登记 KEY 映射表。 */
void Enroll_KEY_Register(void);

/* 软件 I2C 注册：按板级映射绑定两根线到 bit-bang 驱动。 */
void Enroll_I2C_Register(void);

/* 软件 SPI 注册：按板级映射绑定四根线到 bit-bang 驱动。 */
void Enroll_SPI_Register(void);

/* OLED 注册：注册 SPI 模式下的 DC/RES 板级控制引脚。 */
void Enroll_OLED_Register(void);

/* 根据板级映射注册 MPU6050 外部中断与回调。 */
void Enroll_MPU6050_Register(void);

/* PWM 资源注册：按板级映射绑定 API 与 Core。 */
void Enroll_PWM_Register(void);

/* 定时器资源注册。 */
void Enroll_TIM_Register(void);

/* 定时器中断回调注册。 */
void Enroll_TIM_RegisterIrqHandler(API_TIM_IrqHandler_t handler);

/* ADC 资源注册：按板级映射绑定 API 与 Core。 */
void Enroll_ADC_Register(void);

#ifdef __cplusplus
}
#endif

#endif /* __ENROLL_H */

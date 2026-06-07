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

/* 默认MCU */
#ifndef ENROLL_MCU_TARGET
#define ENROLL_MCU_TARGET  ENROLL_MCU_G3507
#endif

/*
 * 头文件依赖规则：
 * - Enroll.h 是对外接口头，只保留“函数声明真正需要”的类型定义。
 * - 其余板级映射/实现依赖下沉到 Enroll_Internal.h。
 */
#include "usart.h"    /* API_USART_IrqHandler_t */
#include "tim.h"      /* API_TIM_IrqHandler_t */

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

#ifdef __cplusplus
extern "C" {
#endif

/* Enroll 层资源登记 */

/* PWM 资源注册：按板级映射绑定 API 与 Core。 */
void Enroll_PWM_Register(void);

/* ADC 资源注册：按板级映射绑定 API 与 Core。 */
void Enroll_ADC_Register(void);

/* 定时器资源注册 */
void Enroll_TIM_Register(void);
/* 定时器中断回调注册。 */
void Enroll_TIM_RegisterIrqHandler(API_TIM_IrqHandler_t handler);

/* 串口资源注册 */
void Enroll_USART_Register(void);
/* 串口中断回调注册 */
void Enroll_USART_RegisterIrqHandler(API_USART_IrqHandler_t handler);

/* 软件 I2C 注册：按板级映射绑定两根线到 bit-bang 驱动。 */
void Enroll_I2C_Register(void);

/* 软件 SPI 注册：按板级映射绑定四根线到 bit-bang 驱动。 */
void Enroll_SPI_Register(void);

/* LED 资源注册：登记 LED 映射表。 */
void Enroll_LED_Register(void);

/* KEY 资源注册：登记 KEY 映射表。 */
void Enroll_KEY_Register(void);

/* OLED 注册：注册 SPI 模式下的 DC/RES 板级控制引脚。 */
void Enroll_OLED_Register(void);

/* TB6612 电机驱动注册：登记方向脚与PWM通道。 */
void Enroll_TB6612_Register(void);

/* MPU6050 外部中断与回调。 */
void Enroll_MPU6050_Register(void);

/* 编码器资源注册：登记板级编码器资源表。 */
void Enroll_Encoder_Register(void);

#ifdef __cplusplus
}
#endif

#endif /* __ENROLL_H */




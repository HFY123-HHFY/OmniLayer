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

#ifndef ENROLL_MCU_G3507
#define ENROLL_MCU_G3507  2U
#endif

/*
 * 全局宏重定义 ENROLL_MCU_TARGET。
 */
#ifndef ENROLL_MCU_TARGET
#define ENROLL_MCU_TARGET  ENROLL_MCU_G3507
#endif

#include "LED.h"
#include "gpio.h"
#include "pwm.h"
#include "usart.h"
#include "tim.h"
#include "adc.h"

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

/*
 * Enroll 层只负责"登记"和"对外转发接口"。
 * 这里的 LED 接口会把板级硬件映射和 Core 的 GPIO 驱动连接起来。
 */
void Enroll_LED_Init(LED_Level_t initLevel);

/* 把 app/main 的 LED 控制请求转发给 BSP。 */
void Enroll_LED_Control(LED_Id_t id, LED_Level_t level);

/* 串口注册并初始化：按板级映射绑定 API 与 Core。 */
void Enroll_USART_Init(API_USART_Id_t id, uint32_t baudRate);
void Enroll_USART_RegisterIrqHandler(API_USART_IrqHandler_t handler);

/* PWM 注册并初始化：按板级映射绑定 API 与 Core。 */
void Enroll_PWM_Init(API_PWM_Tim_t timId, uint16_t arr, uint16_t psc);

/* 定时器注册并绑定统一中断回调。 */
void Enroll_TIM_RegisterIrqHandler(API_TIM_IrqHandler_t handler);

/* ADC 注册并初始化：按板级映射绑定 API 与 Core。 */
void Enroll_ADC_Init(API_ADC_Id_t id);

#ifdef __cplusplus
}
#endif

#endif /* __ENROLL_H */


















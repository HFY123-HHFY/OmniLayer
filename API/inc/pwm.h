#ifndef __API_PWM_H
#define __API_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PWM 定时器编号常量。 */
typedef enum
{
	API_PWM_TIM1 = 1U,
	API_PWM_TIM2 = 2U,
	API_PWM_TIM3 = 3U,
	API_PWM_TIM4 = 4U,
	API_PWM_TIM5 = 5U,
	API_PWM_TIM9 = 9U
} API_PWM_Tim_t;

/* PWM 通道编号常量。 */
typedef enum
{
	API_PWM_CH1 = 1U,
	API_PWM_CH2 = 2U,
	API_PWM_CH3 = 3U,
	API_PWM_CH4 = 4U
} API_PWM_Channel_t;

typedef struct
{
	/* 选择哪个定时器输出 PWM。 */
	API_PWM_Tim_t timId;
	/* 该定时器的通道号。 */
	API_PWM_Channel_t channel;
	/* 映射到的 GPIO 端口。 */
	void *port;
	/* 映射到的 GPIO 引脚（位掩码）。 */
	uint16_t pin;
} API_PWM_Config_t;

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_pwm.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_pwm.h"
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

/* 注册板级 PWM 引脚映射表。 */
void API_PWM_Register(const API_PWM_Config_t *configTable, uint8_t count);

/*
 * PWM 初始化函数：
 * timId -> 选择哪个定时器
 * arr   -> 自动重装载值
 * psc   -> 预分频值
 */
void API_PWM_Init(API_PWM_Tim_t timId, uint16_t arr, uint16_t psc);

/*
 * 设置比较值函数：
 * timId   -> 选择哪个定时器
 * channel -> 选择该定时器哪个通道
 * ccr     -> 该通道比较寄存器值
 */
void API_PWM_Setcom(API_PWM_Tim_t timId, API_PWM_Channel_t channel, uint16_t ccr);

#ifdef __cplusplus
}
#endif

#endif /* __API_PWM_H */

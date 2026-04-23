#ifndef __API_PWM_H
#define __API_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	/* 选择哪个定时器输出 PWM，例如 2/3/9。 */
	uint8_t timId;
	/* 该定时器的通道号，范围 1~4。 */
	uint8_t channel;
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
void API_PWM_Init(uint8_t timId, uint16_t arr, uint16_t psc);

/*
 * 设置比较值函数：
 * timId   -> 选择哪个定时器
 * channel -> 选择该定时器哪个通道
 * ccr     -> 该通道比较寄存器值
 */
void API_PWM_Setcom(uint8_t timId, uint8_t channel, uint16_t ccr);

#ifdef __cplusplus
}
#endif

#endif /* __API_PWM_H */

#ifndef __API_TIM_H
#define __API_TIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_tim.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_tim.h"
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

typedef enum
{
	API_TIM1 = 0U,
	API_TIM2 = 1U,
	API_TIM3 = 2U,
	API_TIM4 = 3U,
	API_TIM5 = 4U
} API_TIM_Id_t;

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
typedef struct
{
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t SMCR;
	volatile uint32_t DIER;
	volatile uint32_t SR;
	volatile uint32_t EGR;
	volatile uint32_t CCMR1;
	volatile uint32_t CCMR2;
	volatile uint32_t CCER;
	volatile uint32_t CNT;
	volatile uint32_t PSC;
	volatile uint32_t ARR;
} F103_TIM_View_t;

#define TIM2 ((F103_TIM_View_t *)0x40000000UL)
#define TIM3 ((F103_TIM_View_t *)0x40000400UL)
#define TIM4 ((F103_TIM_View_t *)0x40000800UL)
#define TIM_SR_UIF (1UL << 0)
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
typedef struct
{
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t SMCR;
	volatile uint32_t DIER;
	volatile uint32_t SR;
	volatile uint32_t EGR;
	volatile uint32_t CCMR1;
	volatile uint32_t CCMR2;
	volatile uint32_t CCER;
	volatile uint32_t CNT;
	volatile uint32_t PSC;
	volatile uint32_t ARR;
} F407_TIM_View_t;

#define TIM2 ((F407_TIM_View_t *)0x40000000UL)
#define TIM3 ((F407_TIM_View_t *)0x40000400UL)
#define TIM4 ((F407_TIM_View_t *)0x40000800UL)
#define TIM5 ((F407_TIM_View_t *)0x40000C00UL)
#define TIM_SR_UIF (1UL << 0)
#endif

/*
 * 定时器初始化接口：
 * id 选择定时器实例，periodMs 指定中断周期（毫秒）。
 */
void API_TIM_Init(API_TIM_Id_t id, uint32_t periodMs);

#ifdef __cplusplus
}
#endif

#endif /* __API_TIM_H */

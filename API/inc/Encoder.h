#ifndef __API_ENCODER_H
#define __API_ENCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * API Encoder 层职责：
 * 1) 提供统一的编码器接口（读速度）；
 * 2) 通过条件编译分发到 Core 层实现；
 * 3) 底层差异：F103/F407 用定时器编码器模式，G3507 用外部中断模拟。
 */

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_Encoder.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_Encoder.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#include "G3507_Encoder.h"
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

/* 逻辑编码器 ID */
typedef enum
{
	API_ENCODER_1 = 0U,
	API_ENCODER_2 = 1U
} API_Encoder_Id_t;

/* 编码器速度全局变量 */
extern int16_t Encoder1_Speed;
extern int16_t Encoder2_Speed;

/*
 * 定时器输入捕获通道常量
 * 用于 hw_config 中指定编码器信号映射到哪个 TIM 通道。
 */
#define API_ENCODER_CH1  (1U)
#define API_ENCODER_CH2  (2U)

/*
 * Core 编码器 ID：
 * F103: 用 TIM 编号（TIM2=1, TIM3=2, TIM4=3）区分
 * F407: 用 TIM 编号（TIM1=0, TIM2=1, TIM3=2, TIM4=3, TIM5=4, TIM8=5）区分
 * G3507: 用编码器序号（0/1）区分
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#define API_ENCODER_CORE_TIM2  (1U)
#define API_ENCODER_CORE_TIM3  (2U)
#define API_ENCODER_CORE_TIM4  (3U)
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#define API_ENCODER_CORE_TIM1  (0U)
#define API_ENCODER_CORE_TIM2  (1U)
#define API_ENCODER_CORE_TIM3  (2U)
#define API_ENCODER_CORE_TIM4  (3U)
#define API_ENCODER_CORE_TIM5  (4U)
#define API_ENCODER_CORE_TIM8  (5U)
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#define API_ENCODER_CORE_ENC0  (0U)
#define API_ENCODER_CORE_ENC1  (1U)
#endif

/*
 * 编码器配置表项：
 * - id:     逻辑编码器 ID
 * - coreId: Core 层实例 ID（TIM 编号或编码器序号）
 * - chA:    A 相信号对应的定时器输入捕获通道（CH1/CH2）
 * - portA/pinA: A 相 GPIO
 * - chB:    B 相信号对应的定时器输入捕获通道（CH1/CH2）
 * - portB/pinB: B 相 GPIO
 */
typedef struct
{
	API_Encoder_Id_t id;
	uint8_t          coreId;
	uint8_t          chA;
	void            *portA;
	uint32_t         pinA;
	uint8_t          chB;
	void            *portB;
	uint32_t         pinB;
} API_Encoder_Config_t;

/*
 * 编码器注册：登记板级编码器资源表（不初始化硬件）。
 */
void API_Encoder_Register(const API_Encoder_Config_t *configTable, uint8_t count);

/*
 * 编码器初始化：根据注册表启动指定编码器硬件。
 */
void API_Encoder_Init(API_Encoder_Id_t id);

/*
 * 读取编码器速度：返回自上次读取后的计数值增量（带方向），并清零计数器。
 * 调用方应在固定周期内调用以获得速度意义。
 */
int16_t API_Encoder_GetSpeed(API_Encoder_Id_t id);

#ifdef __cplusplus
}
#endif

#endif /* __API_ENCODER_H */

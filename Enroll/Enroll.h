#ifndef __ENROLL_H
#define __ENROLL_H

#include "LED.h"

/* MCU 目标选择宏：0=F103, 1=F407, 3=其他 MCU 预留。 */
#define ENROLL_MCU_F103   0U
#define ENROLL_MCU_F407   1U
#define ENROLL_MCU_OTHER  3U

/*
 * 用户可在工程全局宏中重定义 ENROLL_MCU_TARGET。
 * 默认F103。
 */
#ifndef ENROLL_MCU_TARGET
#define ENROLL_MCU_TARGET  ENROLL_MCU_F103
#endif

/*
 * 条件编译选择不同 MCU 的 hw_config 和 gpio 驱动。
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "103_hw_config.h"
#include "f103_gpio.h"
#define ENROLL_GPIO_INIT_FN   F103_GPIO_InitOutput
#define ENROLL_GPIO_WRITE_FN  F103_GPIO_Write
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "407_hw_config.h"
#include "f407_gpio.h"
#define ENROLL_GPIO_INIT_FN   F407_GPIO_InitOutput
#define ENROLL_GPIO_WRITE_FN  F407_GPIO_Write
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_OTHER)
/* 这里给其他 MCU 预留：例如 include "xxx_hw_config.h" 和 "xxx_gpio.h" */
#include "other_hw_config.h"
#include "other_gpio.h"
#define ENROLL_GPIO_INIT_FN   OTHER_GPIO_InitOutput
#define ENROLL_GPIO_WRITE_FN  OTHER_GPIO_Write
#else
#error "Unsupported ENROLL_MCU_TARGET. Use 0(F103), 1(F407), or 3(OTHER)."
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* __ENROLL_LED_H */

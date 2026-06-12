#ifndef __ENROLL_INTERNAL_H
#define __ENROLL_INTERNAL_H

/*
 * Enroll Internal 层职责：
 * 1) 提供 Enroll.c 专用的内部依赖；
 * 2) 存放只供注册实现使用的头文件与宏；
 * 3) 不建议被 App/其他模块直接 include。
 */

#include "Enroll.h"

/*
 * 内部实现依赖：
 * 这些头文件只服务于 Enroll.c 的注册实现，不属于对外接口。
 */
#include "LED.h"
#include "KEY.h"
#include "gpio.h"
#include "API_I2C.h"
#include "API_SPI.h"
#include "pwm.h"
#include "adc.h"
#include "usart.h"
#include "tim.h"
#include "OLED.h"
#include "exti.h"
#include "MPU6050_Int.h"
#include "TB6612.h"
#include "Encoder.h"

#include <stddef.h>

/*
 * GPIO 统一经 API 层分发到对应 Core 实现。
 * 仅 Enroll.c 内部展开 LED/KEY 配置表时使用。
 */
#define ENROLL_GPIO_INIT_FN   API_GPIO_InitOutput
#define ENROLL_GPIO_INPUT_FN  API_GPIO_InitInputPullUp
#define ENROLL_GPIO_WRITE_FN  API_GPIO_Write
#define ENROLL_GPIO_READ_FN   API_GPIO_Read

#endif /* __ENROLL_INTERNAL_H */

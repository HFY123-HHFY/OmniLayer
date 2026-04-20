#ifndef __API_GPIO_H
#define __API_GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_gpio.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_gpio.h"
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

/* 配置 GPIO 为推挽输出。 */
void API_GPIO_InitOutput(void *port, uint16_t pin);
/* 配置 GPIO 为输入模式。 */
void API_GPIO_InitInput(void *port, uint16_t pin);
/* GPIO 输出电平控制。 */
void API_GPIO_Write(void *port, uint16_t pin, uint8_t level);
/* GPIO 输入电平读取。 */
uint8_t API_GPIO_Read(void *port, uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __API_GPIO_H */

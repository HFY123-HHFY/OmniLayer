#ifndef __F407_ADC_H
#define __F407_ADC_H

#include <stdint.h>

#include "f407_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 F407 ADC 指定通道和对应模拟输入引脚。 */
void F407_ADC_InitChannel(uint8_t adcId, uint8_t channel, void *port, uint16_t pin);
/* 软件触发一次转换并返回采样值。 */
uint16_t F407_ADC_ReadChannel(uint8_t adcId, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* __F407_ADC_H */

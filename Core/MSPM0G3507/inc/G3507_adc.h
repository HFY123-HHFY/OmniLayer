#ifndef __G3507_ADC_H
#define __G3507_ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void G3507_ADC_InitChannel(uint8_t adcId, uint8_t channel, void *port, uint32_t pin);
uint16_t G3507_ADC_ReadChannel(uint8_t adcId, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_ADC_H */

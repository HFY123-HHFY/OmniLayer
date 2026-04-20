#ifndef __F407_USART_H
#define __F407_USART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void F407_USART_Init(uint8_t usartId, uint32_t baudRate);
void F407_USART_WriteByte(uint8_t usartId, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* __F407_USART_H */

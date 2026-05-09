#ifndef __G3507_USART_H
#define __G3507_USART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 串口初始化：usartId=0/1/2 对应 UART0/1/2。 */
void G3507_USART_Init(uint8_t usartId, uint32_t baudRate);

/* 串口发送 1 字节（阻塞直到硬件可发送）。 */
void G3507_USART_WriteByte(uint8_t usartId, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_USART_H */

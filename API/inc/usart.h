#ifndef __API_USART_H
#define __API_USART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_usart.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_usart.h"
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

typedef enum
{
	API_USART1 = 0U,
	API_USART2 = 1U,
	API_USART3 = 2U,
	API_USART4 = 3U
} API_USART_Id_t;

typedef struct
{
	API_USART_Id_t id;
	void *txPort;
	uint16_t txPin;
	void *rxPort;
	uint16_t rxPin;
} API_USART_Config_t;

/*
 * 供应用层在串口中断里直接访问的最小寄存器视图。
 * 这样 main.c 可以直接写 USARTx_IRQHandler，而不用再走额外的 API 串口 IRQ 包装。
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
typedef struct
{
	volatile uint32_t SR;
	volatile uint32_t DR;
	volatile uint32_t BRR;
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t CR3;
	volatile uint32_t GTPR;
} F103_USART_View_t;

#define USART1 ((F103_USART_View_t *)0x40013800UL)
#define USART2 ((F103_USART_View_t *)0x40004400UL)
#define USART3 ((F103_USART_View_t *)0x40004800UL)
#define USART_SR_RXNE (1UL << 5)
#define USART_SR_TC   (1UL << 6)
#define USART_SR_TXE  (1UL << 7)
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
typedef struct
{
	volatile uint32_t SR;
	volatile uint32_t DR;
	volatile uint32_t BRR;
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t CR3;
	volatile uint32_t GTPR;
} F407_USART_View_t;

#define USART1 ((F407_USART_View_t *)0x40011000UL)
#define USART2 ((F407_USART_View_t *)0x40004400UL)
#define USART3 ((F407_USART_View_t *)0x40004800UL)
#define USART4 ((F407_USART_View_t *)0x40004C00UL)
#define USART_SR_RXNE (1UL << 5)
#define USART_SR_TC   (1UL << 6)
#define USART_SR_TXE  (1UL << 7)
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

void API_USART_Register(const API_USART_Config_t *configTable, uint8_t count);
/* 串口初始化接口：id 选择串口，baudRate 设置波特率。 */
void API_USART_Init(API_USART_Id_t id, uint32_t baudRate);
/* 串口发送 1 字节。 */
void API_USART_WriteByte(API_USART_Id_t id, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* __API_USART_H */

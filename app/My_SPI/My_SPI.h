#ifndef __MY_SPI_H
#define __MY_SPI_H

#include <stdint.h>

#include "gpio.h"

typedef struct
{
	void *port;
	uint16_t csPin;
	uint16_t sckPin;
	uint16_t mosiPin;
	uint16_t misoPin;
} MySPI_Config_t;

/* 注册板级 SPI 配置表。 */
void MySPI_Register(const MySPI_Config_t *configTable, uint8_t count);

/* 软件 SPI 初始化（模式0默认空闲：CS=1, SCK=0）。 */
void MySPI_Init(void);

/* 引脚电平控制接口（协议层调用）。 */
void MySPI_W_SS(uint8_t bitValue);
void MySPI_W_SCK(uint8_t bitValue);
void MySPI_W_MOSI(uint8_t bitValue);
uint8_t MySPI_R_MISO(void);

/* 协议层接口。 */
void MySPI_Start(void);
void MySPI_Stop(void);
uint8_t MySPI_SwapByte(uint8_t byteSend);

/*
 * 最小 SPI 测试例程：
 * - 发送一组测试字节并打印收发结果。
 * - 若未挂从机，可临时短接 MOSI-MISO 做回环验证。
 */
void App_SPI_TestOnce(void);

#endif /* __MY_SPI_H */
#ifndef __MY_SPI_H
#define __MY_SPI_H

#endif /* __MY_SPI_H */

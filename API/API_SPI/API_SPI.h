#ifndef __API_SPI_H
#define __API_SPI_H

#include <stdint.h>

/*
 * API_SPI 说明:
 * - 协议层 (本文件): 平台无关的 SPI 协议逻辑, 始终编译。
 * - 底层实现: Core/{platform}/{platform}_soft_spi.c 提供 GPIO 翻转与延时。
 * - 引脚映射由 Enroll 注册层提供。
 * - 协议时序采用软件模拟 (bit-bang, SPI 模式 0)。
 * - 推荐调用顺序:
 *   1) Enroll_SPI_Register()
 *   2) API_SPI_Init()
 *   3) API_SPI_SelectBus() / API_SPI_SetSpeed()
 *   4) API_SPI_Start() / API_SPI_SwapByte() / API_SPI_Stop()
 */

typedef struct
{
	/* SPI 总线编号。 */
	uint8_t id;
	/* CS 片选端口/引脚。 */
	void *csPort;
	uint32_t csPin;
	uint32_t csIomux; /* G3507 IOMUX PINCM index; 0 for STM32 */
	/* SCK 时钟端口/引脚。 */
	void *sckPort;
	uint32_t sckPin;
	uint32_t sckIomux; /* G3507 IOMUX PINCM index; 0 for STM32 */
	/* MOSI 主发从收端口/引脚。 */
	void *mosiPort;
	uint32_t mosiPin;
	uint32_t mosiIomux; /* G3507 IOMUX PINCM index; 0 for STM32 */
	/* MISO 主收从发端口/引脚。 */
	void *misoPort;
	uint32_t misoPin;
	uint32_t misoIomux; /* G3507 IOMUX PINCM index; 0 for STM32 */
} API_SPI_Config_t;

typedef enum
{
	API_SPI1 = 0,
	API_SPI2,
	/* 总线数量上界/非法值哨兵, 不作为真实总线使用。 */
	API_SPI_MAX
} API_SPI_BusId_t;

/* 注册板级 SPI 配置表。 */
void API_SPI_Register(const API_SPI_Config_t *configTable, uint8_t count);
/* 选择当前操作的软件 SPI 总线。 */
void API_SPI_SelectBus(API_SPI_BusId_t busId);

/* 软件 SPI 初始化 (模式 0 默认空闲: CS=1, SCK=0)。 */
void API_SPI_Init(void);

/*
 * 软件 SPI 速率档位
 */
typedef enum
{
	API_SPI_SPEED_250K = 0,
	API_SPI_SPEED_500K,
	API_SPI_SPEED_1M,
	API_SPI_SPEED_2M,
	API_SPI_SPEED_5M
} API_SPI_SpeedTypeDef;

/* 设置软件 SPI 速率档位。 */
void API_SPI_SetSpeed(API_SPI_SpeedTypeDef speed);
/* 获取当前软件 SPI 速率档位。 */
API_SPI_SpeedTypeDef API_SPI_GetSpeed(void);
/* 关闭/开启 bit-bang 延时 (高速设备如 OLED 可关闭延时以全速运行)。 */
void API_SPI_DelayOff(void);
void API_SPI_DelayOn(void);

/* 协议层接口。 */
void API_SPI_Start(void);
void API_SPI_Stop(void);
uint8_t API_SPI_SwapByte(uint8_t byteSend);

/*
 * 最小 SPI 测试例程:
 * - 发送一组测试字节并打印收发结果。
 * - 若未挂从机, 可临时短接 MOSI-MISO 做回环验证。
 */
void App_SPI_TestOnce(void);

#endif /* __API_SPI_H */

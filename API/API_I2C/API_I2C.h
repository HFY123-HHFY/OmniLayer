#ifndef __API_I2C_H
#define __API_I2C_H

#include <stdint.h>

/*
 * API_I2C 说明:
 * - 协议层 (本文件): 平台无关的 I2C 协议逻辑, 始终编译。
 * - 底层实现: Core/{platform}/{platform}_soft_i2c.c 提供 GPIO 翻转与延时。
 * - 引脚映射由 Enroll 注册层提供。
 * - 推荐调用顺序:
 *   1) Enroll_I2C_Register()
 *   2) API_I2C_Init()
 *   3) API_I2C_SelectBus() / API_I2C_SetSpeed()
 *   4) API_I2C_Start() / SendByte() / Wait_Ack() / Stop()
 */

typedef struct
{
	uint8_t id;
	void *sclPort;
	uint32_t sclPin;
	uint32_t sclIomux; /* G3507 IOMUX PINCM index; 0 for STM32 */
	void *sdaPort;
	uint32_t sdaPin;
	uint32_t sdaIomux; /* G3507 IOMUX PINCM index; 0 for STM32 */
} API_I2C_Config_t;

typedef enum
{
	API_I2C1 = 0,
	API_I2C2,
	/* 总线数量上界/非法值哨兵, 不作为真实总线使用。 */
	API_I2C_MAX
} API_I2C_BusId_t;

typedef enum
{
	API_I2C_SPEED_50K = 0,
	API_I2C_SPEED_100K,
	API_I2C_SPEED_200K,
	API_I2C_SPEED_400K
} API_I2C_SpeedTypeDef;

/* 统一 ACK 等待超时轮次, 三平台共用。 */
#define API_I2C_ACK_TIMEOUT_COUNT (16U)

/* 注册板级 I2C 配置表。 */
void API_I2C_Register(const API_I2C_Config_t *configTable, uint8_t count);
/* 选择当前操作的软件 I2C 总线。 */
void API_I2C_SelectBus(API_I2C_BusId_t busId);
/* 初始化软件 I2C 并释放总线到空闲态 (SCL=1, SDA=1)。 */
void API_I2C_Init(void);
/* 设置软件 I2C 速率档位。 */
void API_I2C_SetSpeed(API_I2C_SpeedTypeDef speed);
/* 获取当前软件 I2C 速率档位。 */
API_I2C_SpeedTypeDef API_I2C_GetSpeed(void);
/* 关闭/开启 bit-bang 延时 (高速设备如 OLED 可关闭延时以全速运行)。 */
void API_I2C_DelayOff(void);
void API_I2C_DelayOn(void);

/* I2C 起始条件。 */
void API_I2C_Start(void);
/* I2C 停止条件。 */
void API_I2C_Stop(void);
/* 发送 1 个字节 (MSB first)。 */
void API_I2C_SendByte(uint8_t Byte);
/* 接收 1 个字节, Ack=1 发送 ACK, Ack=0 发送 NACK。 */
uint8_t API_I2C_ReceiveByte(unsigned char Ack);
/* 发送 ACK。 */
void API_I2C_Ack(void);
/* 发送 NACK。 */
void API_I2C_NAck(void);
/* 等待从机 ACK: 返回 0=收到 ACK, 1=超时失败。 */
uint8_t API_I2C_Wait_Ack(void);

/* 最小 I2C 测试例程: 遍历已注册总线并逐路扫描 7-bit 地址空间 */
void App_I2C_ScanOnce(void);

#endif /* __API_I2C_H */

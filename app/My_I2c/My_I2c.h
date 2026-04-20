#ifndef __MY_I2C_H
#define __MY_I2C_H

#include "gpio.h"
#include <stdint.h>

/*
 * My_I2C说明：
 * - 引脚映射由 Enroll 注册层提供。
 * - 协议时序采用软件模拟（bit-bang）。
 * - 推荐调用顺序：
 *   1) Enroll_I2C_Register()
 *   2) MyI2C_Init()
 *   3) MyI2C_Start()/SendByte()/Wait_Ack()/ReceiveByte()/Stop()
 */

typedef struct
{
	void *port;
	uint16_t sclPin;
	uint16_t sdaPin;
} MyI2C_Config_t;

/* 注册板级 I2C 配置表。 */
void MyI2C_Register(const MyI2C_Config_t *configTable, uint8_t count);
/* 初始化软件 I2C 并释放总线到空闲态（SCL=1, SDA=1）。 */
void MyI2C_Init(void);

/* 写 SCL 电平（0/1）。 */
void MyI2C_W_SCL(uint8_t BitValue);
/* 写 SDA 电平（0/1）。 */
void MyI2C_W_SDA(uint8_t BitValue);
/* 读 SDA 电平（返回 0 或 1）。 */
uint8_t MyI2C_R_SDA(void);

/* 将 SDA 切换为输入模式（用于 ACK/读数据阶段）。 */
void MyI2C_Set_SDA_Input(void);
/* 将 SDA 切换为输出模式（用于起始/停止/写数据阶段）。 */
void MyI2C_Set_SDA_Output(void);

/* I2C 起始条件。 */
void MyI2C_Start(void);
/* I2C 停止条件。 */
void MyI2C_Stop(void);
/* 发送 1 个字节（MSB first）。 */
void MyI2C_SendByte(uint8_t Byte);
/* 接收 1 个字节，Ack=1 发送 ACK，Ack=0 发送 NACK。 */
uint8_t MyI2C_ReceiveByte(unsigned char Ack);
/* 发送 ACK。 */
void MyI2C_Ack(void);
/* 发送 NACK。 */
void MyI2C_NAck(void);
/* 等待从机 ACK：返回 0=收到 ACK，1=超时失败。 */
uint8_t MyI2C_Wait_Ack(void);

/* 最小 I2C 测试例程：扫描 7-bit 地址空间，发送写地址并检测 ACK */
void App_I2C_ScanOnce(void);

#endif /* __MY_I2C_H */

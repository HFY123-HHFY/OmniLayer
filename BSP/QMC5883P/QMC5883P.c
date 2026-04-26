#include "QMC5883P.h"

#include "My_I2c.h"
#include <math.h>

/* 最近一次角度结果（单位：度，0~360）。 */
float Angle_XY = 0.0f;

/*  选择I2C1 设置QMC5883P I2C速率为100kHZ */
static void QMC_SelectI2CSpeed(void)
{
	MyI2C_SelectBus(QMC5883P_I2C_BUS);
	MyI2C_SetSpeed(QMC5883P_I2C_SPEED);
}

/* 向指定寄存器写 1 字节。 */
static void QMC_WriteReg(uint8_t regAddress, uint8_t data)
{
	QMC_SelectI2CSpeed();
	MyI2C_Start();
	MyI2C_SendByte(QMC5883P_I2C_ADDR_W);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(regAddress);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(data);
	MyI2C_Wait_Ack();
	MyI2C_Stop();
}

/* 从指定寄存器读取 1 字节。 */
static uint8_t QMC_ReadReg(uint8_t regAddress)
{
	uint8_t data;

	QMC_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte(QMC5883P_I2C_ADDR_W);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(regAddress);
	MyI2C_Wait_Ack();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)(QMC5883P_I2C_ADDR_W | 0x01U));
	MyI2C_Wait_Ack();
	data = MyI2C_ReceiveByte(0U);
	MyI2C_NAck();
	MyI2C_Stop();

	return data;
}

/* 读取芯片 ID。 */
uint8_t QMC_GetID(void)
{
	return QMC_ReadReg(QMC5883P_REG_CHIPID);
}

/*
 * 传感器初始化：
 * - CONTROL1 = 0xFF：连续模式 + 200Hz（沿用用户提供配置）
 * - CONTROL2 = 0x01：保持默认软复位/自检相关位配置
 */
void QMC_Init(void)
{
	QMC_WriteReg(QMC5883P_REG_CONTROL1, 0xFFU);
	QMC_WriteReg(QMC5883P_REG_CONTROL2, 0x01U);
}

/* 读取三轴原始磁场数据。 */
void QMC_GetData(int16_t *magX, int16_t *magY, int16_t *magZ)
{
	uint8_t dataH;
	uint8_t dataL;

	dataH = QMC_ReadReg(QMC5883P_REG_XOUT_H);
	dataL = QMC_ReadReg(QMC5883P_REG_XOUT_L);
	*magX = (int16_t)(((uint16_t)dataH << 8) | dataL);

	dataH = QMC_ReadReg(QMC5883P_REG_YOUT_H);
	dataL = QMC_ReadReg(QMC5883P_REG_YOUT_L);
	*magY = (int16_t)(((uint16_t)dataH << 8) | dataL);

	dataH = QMC_ReadReg(QMC5883P_REG_ZOUT_H);
	dataL = QMC_ReadReg(QMC5883P_REG_ZOUT_L);
	*magZ = (int16_t)(((uint16_t)dataH << 8) | dataL);
}

/* 计算 XY 平面角度并返回。 */
float QMC_Data(void)
{
	int16_t x = 0;
	int16_t y = 0;
	int16_t z = 0;

	static float Angle_XY_temp = 0.0f;

	QMC_GetData(&x, &y, &z);
	(void)z;

	/* atan2f 输出范围为 [-180,180]，加 180 后映射到 [0,360]。 */
	Angle_XY_temp = atan2f((float)y, (float)x) * 57.2957795f + 180.0f;
	Angle_XY = Angle_XY_temp;
	return Angle_XY_temp;
}

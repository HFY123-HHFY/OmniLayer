#include "MPU6050.h"

#include "My_I2c.h"
#include "Delay.h"

/*
 * MPU6050 驱动代码
 */

/* 坐标系修正: 与 DMP 的 gyro_orientation 保持一致（背面安装，绕 X 轴翻转180度） */

/* 选择I2C1 设置MPU6050 I2C速率为400kHZ */
static void MPU_SelectI2CSpeed(void)
{
	MyI2C_SelectBus(MPU6050_I2C_BUS);
	MyI2C_SetSpeed(MPU6050_I2C_SPEED);
}

/*
正面默认是这样的：
{
    (void)x;
	*y = (int16_t)(*y);
	*z = (int16_t)(*z);
}
*/

static void MPU_Apply_Mount_Transform(int16_t *x, int16_t *y, int16_t *z)
{
	(void)x;
	*y = (int16_t)(-*y);
	*z = (int16_t)(-*z);
}

/* 初始化 MPU6050
 * 返回值: 0 成功, 1 失败
 */
uint8_t MPU_Init(void)
{
	uint8_t res;

	/* 复位并唤醒 */
	MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x80U);
	Delay_ms(100U);
	MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x00U);

	/* 量程与采样率 */
	MPU_Set_Gyro_Fsr(3U);
	MPU_Set_Accel_Fsr(0U);
	MPU_Set_Rate(50U);

	/* 基础功能配置 */
	MPU_Write_Byte(MPU_INT_EN_REG, 0x00U);
	MPU_Write_Byte(MPU_USER_CTRL_REG, 0x00U);
	MPU_Write_Byte(MPU_FIFO_EN_REG, 0x00U);
	MPU_Write_Byte(MPU_INTBP_CFG_REG, 0x80U);

	res = MPU_Read_Byte(MPU_DEVICE_ID_REG);
	if (res == MPU_ADDR)
	{
		MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x01U);
		MPU_Write_Byte(MPU_PWR_MGMT2_REG, 0x00U);
		MPU_Set_Rate(50U);
		return 0U;
	}

	return 1U;
}

/* 设置陀螺仪满量程: 0/1/2/3 -> ±250/500/1000/2000dps */
uint8_t MPU_Set_Gyro_Fsr(uint8_t fsr)
{
	return MPU_Write_Byte(MPU_GYRO_CFG_REG, (uint8_t)(fsr << 3));
}

/* 设置加速度满量程: 0/1/2/3 -> ±2/4/8/16g */
uint8_t MPU_Set_Accel_Fsr(uint8_t fsr)
{
	return MPU_Write_Byte(MPU_ACCEL_CFG_REG, (uint8_t)(fsr << 3));
}

/* 设置数字低通滤波器频率 */
uint8_t MPU_Set_LPF(uint16_t lpf)
{
	uint8_t data;

	if (lpf >= 188U) { data = 1U; }
	else if (lpf >= 98U) { data = 2U; }
	else if (lpf >= 42U) { data = 3U; }
	else if (lpf >= 20U) { data = 4U; }
	else if (lpf >= 10U) { data = 5U; }
	else { data = 6U; }

	return MPU_Write_Byte(MPU_CFG_REG, data);
}

/* 设置采样率(Hz), 有效范围 4~1000 */
uint8_t MPU_Set_Rate(uint16_t rate)
{
	uint8_t div;

	if (rate > 1000U) { rate = 1000U; }
	if (rate < 4U) { rate = 4U; }

	div = (uint8_t)(1000U / rate - 1U);
	MPU_Write_Byte(MPU_SAMPLE_RATE_REG, div);

	/* LPF 跟随采样率一半 */
	return MPU_Set_LPF((uint16_t)(rate / 2U));
}

/* FIFO 使能位配置 */
uint8_t MPU_Set_Fifo(uint8_t sens)
{
	return MPU_Write_Byte(MPU_FIFO_EN_REG, sens);
}

/* 获取温度（返回值扩大 100 倍） */
int16_t MPU_Get_Temperature(void)
{
	uint8_t buf[2];
	int16_t raw;
	float temp;

	MPU_Read_Len(MPU_ADDR, MPU_TEMP_OUTH_REG, 2U, buf);
	raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
	temp = 36.53f + ((float)raw) / 340.0f;

	return (int16_t)(temp * 100.0f);
}

/* 获取陀螺仪原始值 */
uint8_t MPU_Get_Gyroscope(int16_t *gx, int16_t *gy, int16_t *gz)
{
	uint8_t buf[6];
	uint8_t res;

	res = MPU_Read_Len(MPU_ADDR, MPU_GYRO_XOUTH_REG, 6U, buf);
	if (res == 0U)
	{
		*gx = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
		*gy = (int16_t)(((uint16_t)buf[2] << 8) | buf[3]);
		*gz = (int16_t)(((uint16_t)buf[4] << 8) | buf[5]);
		MPU_Apply_Mount_Transform(gx, gy, gz);
	}

	return res;
}

/* 获取加速度原始值 */
uint8_t MPU_Get_Accelerometer(int16_t *ax, int16_t *ay, int16_t *az)
{
	uint8_t buf[6];
	uint8_t res;

	res = MPU_Read_Len(MPU_ADDR, MPU_ACCEL_XOUTH_REG, 6U, buf);
	if (res == 0U)
	{
		*ax = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
		*ay = (int16_t)(((uint16_t)buf[2] << 8) | buf[3]);
		*az = (int16_t)(((uint16_t)buf[4] << 8) | buf[5]);
		MPU_Apply_Mount_Transform(ax, ay, az);
	}

	return res;
}

/* I2C 连续写 */
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	uint8_t i;

	MPU_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)((addr << 1) | 0U));
	if (MyI2C_Wait_Ack())
	{
		MyI2C_Stop();
		return 1U;
	}

	MyI2C_SendByte(reg);
	MyI2C_Wait_Ack();

	for (i = 0U; i < len; i++)
	{
		MyI2C_SendByte(buf[i]);
		if (MyI2C_Wait_Ack())
		{
			MyI2C_Stop();
			return 1U;
		}
	}

	MyI2C_Stop();
	return 0U;
}

/* I2C 连续读 */
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	MPU_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)((addr << 1) | 0U));
	if (MyI2C_Wait_Ack())
	{
		MyI2C_Stop();
		return 1U;
	}

	MyI2C_SendByte(reg);
	MyI2C_Wait_Ack();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)((addr << 1) | 1U));
	MyI2C_Wait_Ack();

	while (len)
	{
		if (len == 1U)
		{
			*buf = MyI2C_ReceiveByte(0U);
		}
		else
		{
			*buf = MyI2C_ReceiveByte(1U);
		}
		len--;
		buf++;
	}

	MyI2C_Stop();
	return 0U;
}

/* I2C 写一个字节 */
uint8_t MPU_Write_Byte(uint8_t reg, uint8_t data)
{
	MPU_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)((MPU_ADDR << 1) | 0U));
	if (MyI2C_Wait_Ack())
	{
		MyI2C_Stop();
		return 1U;
	}

	MyI2C_SendByte(reg);
	MyI2C_Wait_Ack();

	MyI2C_SendByte(data);
	if (MyI2C_Wait_Ack())
	{
		MyI2C_Stop();
		return 1U;
	}

	MyI2C_Stop();
	return 0U;
}

/* I2C 读一个字节 */
uint8_t MPU_Read_Byte(uint8_t reg)
{
	uint8_t res;

	MPU_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)((MPU_ADDR << 1) | 0U));
	MyI2C_Wait_Ack();

	MyI2C_SendByte(reg);
	MyI2C_Wait_Ack();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)((MPU_ADDR << 1) | 1U));
	MyI2C_Wait_Ack();

	res = MyI2C_ReceiveByte(0U);
	MyI2C_Stop();

	return res;
}

#ifndef __QMC5883P_H
#define __QMC5883P_H

#include <stdint.h>
#include "My_I2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* QMC5883P I2C 8-bit 地址（写地址） */
#define QMC5883P_I2C_ADDR_W      0x58U

/* QMC5883P 绑定的软件 I2C 总线与速率。 */
#define QMC5883P_I2C_BUS   My_I2C1
#define QMC5883P_I2C_SPEED I2C_SPEED_100K

/* 寄存器定义 */
#define QMC5883P_REG_CHIPID       0x00U
#define QMC5883P_REG_XOUT_L       0x01U
#define QMC5883P_REG_XOUT_H       0x02U
#define QMC5883P_REG_YOUT_L       0x03U
#define QMC5883P_REG_YOUT_H       0x04U
#define QMC5883P_REG_ZOUT_L       0x05U
#define QMC5883P_REG_ZOUT_H       0x06U
#define QMC5883P_REG_STATUS       0x09U
#define QMC5883P_REG_CONTROL1     0x0AU
#define QMC5883P_REG_CONTROL2     0x0BU

/* 最近一次计算得到的 XY 平面航向角（单位：度，范围 0~360） */
extern float Angle_XY;

/*
 * 初始化 QMC5883P：
 * - 配置连续测量模式
 * - 设置输出数据率与基本工作参数
 */
void QMC_Init(void);

/* 读取芯片 ID（寄存器 0x00）。 */
uint8_t QMC_GetID(void);

/* 读取三轴原始磁场数据（单位为原始计数值）。 */
void QMC_GetData(int16_t *magX, int16_t *magY, int16_t *magZ);

/*
 * 计算 XY 平面角度（0~360 度）：
 * - 使用 atan2f(y, x)
 * - 结果同步写入全局变量 Angle_XY
 */
float QMC_Data(void);

#ifdef __cplusplus
}
#endif


#endif /* __QMC5883P_H */

#ifndef __SOFT_I2C_HAL_H
#define __SOFT_I2C_HAL_H

#include <stdint.h>

/*
 * soft_i2c_hal.h — 软件 I2C 硬件抽象层接口 (内部桥接头文件)
 *
 * 定位: API 协议层 ↔ Core 底层实现 之间的内部桥梁。
 *       BSP/App 层不应直接引用本头文件，应使用 API_I2C.h。
 *
 * 职责: 声明平台无关的 GPIO 翻转与延时原语。
 * 实现: 由 Core/{platform}/{platform}_soft_i2c.c 按平台提供。
 *
 * 本头文件不含任何平台条件编译。
 */

/* GPIO 引脚一次性初始化并预计算全部寄存器缓存值。 */
void soft_i2c_hal_init(void *sclPort, uint32_t sclPin, uint32_t sclIomux,
                       void *sdaPort, uint32_t sdaPin, uint32_t sdaIomux);

/* 写 SCL 电平 (0 或非 0)。 */
void soft_i2c_hal_w_scl(uint8_t bit);
/* 写 SDA 电平 (0 或非 0)。自动确保 SDA 为输出模式。 */
void soft_i2c_hal_w_sda(uint8_t bit);
/* 读 SDA 电平, 返回 0 或 1。 */
uint8_t soft_i2c_hal_r_sda(void);

/* SDA 方向切换: 输入(上拉, 用于 ACK/读) / 输出(推挽, 用于起始/停止/写)。 */
void soft_i2c_hal_set_sda_input(void);
void soft_i2c_hal_set_sda_output(void);

/* 基础延时 (us), 内部检查延时关闭标志。 */
void soft_i2c_hal_delay_us(uint32_t us);

/* 速率档位预计算。speedKhz: 50/100/200/400。 */
void soft_i2c_hal_set_speed(uint32_t speedKhz);
/* 关闭/恢复 bit-bang 延时 (OLED 等高速设备可关闭延时全速运行)。 */
void soft_i2c_hal_delay_off(void);
void soft_i2c_hal_delay_on(void);

#endif /* __SOFT_I2C_HAL_H */

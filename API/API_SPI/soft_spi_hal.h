#ifndef __SOFT_SPI_HAL_H
#define __SOFT_SPI_HAL_H

#include <stdint.h>

/*
 * soft_spi_hal.h — 软件 SPI 硬件抽象层接口 (内部桥接头文件)
 *
 * 定位: API 协议层 ↔ Core 底层实现 之间的内部桥梁。
 *       BSP/App 层不应直接引用本头文件，应使用 API_SPI.h。
 *
 * 职责: 声明平台无关的 GPIO 翻转与延时原语。
 * 实现: 由 Core/{platform}/{platform}_soft_spi.c 按平台提供。
 *
 * 本头文件不含任何平台条件编译。
 */

/* GPIO 引脚一次性初始化并预计算全部寄存器缓存值。 */
void soft_spi_hal_init(void *csPort, uint32_t csPin, uint32_t csIomux,
                       void *sckPort, uint32_t sckPin, uint32_t sckIomux,
                       void *mosiPort, uint32_t mosiPin, uint32_t mosiIomux,
                       void *misoPort, uint32_t misoPin, uint32_t misoIomux);

/* 写 CS/SCK/MOSI 电平 (0 或非 0)。 */
void soft_spi_hal_w_cs(uint8_t bit);
void soft_spi_hal_w_sck(uint8_t bit);
void soft_spi_hal_w_mosi(uint8_t bit);

/* 读 MISO 电平, 返回 0 或 1。 */
uint8_t soft_spi_hal_r_miso(void);

/* 基础延时 (us), 内部检查延时关闭标志。 */
void soft_spi_hal_delay_us(uint32_t us);

/* 速率档位预计算。speedKhz: 250/500/1000/2000/5000。 */
void soft_spi_hal_set_speed(uint32_t speedKhz);
/* 关闭/恢复 bit-bang 延时。 */
void soft_spi_hal_delay_off(void);
void soft_spi_hal_delay_on(void);

#endif /* __SOFT_SPI_HAL_H */

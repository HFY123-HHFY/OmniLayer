#ifndef __BMP280_H
#define __BMP280_H

#include <stdbool.h>
#include <stdint.h>

#include "My_I2c.h"
#include "BusRate.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BMP280 I2C 8-bit 设备地址（写地址） */
#define BMP280_ADDR                             (0xECU)
#define BMP280_DEFAULT_CHIP_ID                  (0x58U)

/* BMP280 绑定的软件 I2C 总线与速率。 */
#define BMP280_I2C_BUS   My_I2C1
#define BMP280_I2C_SPEED BMP280_I2C_SPEED_PROFILE

#define BMP280_CHIP_ID                          (0xD0U)
#define BMP280_RST_REG                          (0xE0U)
#define BMP280_STAT_REG                         (0xF3U)
#define BMP280_CTRL_MEAS_REG                    (0xF4U)
#define BMP280_CONFIG_REG                       (0xF5U)
#define BMP280_PRESSURE_MSB_REG                 (0xF7U)
#define BMP280_PRESSURE_LSB_REG                 (0xF8U)
#define BMP280_PRESSURE_XLSB_REG                (0xF9U)
#define BMP280_TEMPERATURE_MSB_REG              (0xFAU)
#define BMP280_TEMPERATURE_LSB_REG              (0xFBU)
#define BMP280_TEMPERATURE_XLSB_REG             (0xFCU)

#define BMP280_SLEEP_MODE                       (0x00U)
#define BMP280_FORCED_MODE                      (0x01U)
#define BMP280_NORMAL_MODE                      (0x03U)

#define BMP280_TEMPERATURE_CALIB_DIG_T1_LSB_REG (0x88U)
#define BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH (24U)
#define BMP280_DATA_FRAME_SIZE                  (6U)

#define BMP280_OVERSAMP_SKIPPED                 (0x00U)
#define BMP280_OVERSAMP_1X                      (0x01U)
#define BMP280_OVERSAMP_2X                      (0x02U)
#define BMP280_OVERSAMP_4X                      (0x03U)
#define BMP280_OVERSAMP_8X                      (0x04U)
#define BMP280_OVERSAMP_16X                     (0x05U)

/* 对外提供：最近一次计算出的海拔高度（m）。 */
extern float alt;

/* 读取 1 个寄存器。 */
uint8_t iicDevReadByte(uint8_t devaddr, uint8_t addr);
/* 写入 1 个寄存器。 */
void iicDevWriteByte(uint8_t devaddr, uint8_t addr, uint8_t data);
/* 连续读取多个寄存器。 */
void iicDevRead(uint8_t devaddr, uint8_t addr, uint8_t len, uint8_t *rbuf);
/* 连续写入多个寄存器。 */
void iicDevWrite(uint8_t devaddr, uint8_t addr, uint8_t len, uint8_t *wbuf);

/* 初始化 BMP280，成功返回 true。 */
bool BMP280Init(void);
/* 获取气压/温度/海拔（hPa / 摄氏度 / m）。 */
void BMP280GetData(float *pressure, float *temperature, float *asl);
/* 仅返回海拔的便捷接口。 */
float BMP_Data(void);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */

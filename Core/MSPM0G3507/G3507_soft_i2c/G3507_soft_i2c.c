#include "G3507_soft_i2c.h"

#include "G3507_gpio.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "Delay.h"

/* ===================== 模块状态变量 ===================== */

/* G3507 寄存器缓存 */
static GPIO_Regs *s_sclReg;
static GPIO_Regs *s_sdaReg;
static uint32_t s_sclPin;
static uint32_t s_sdaPin;
static uint32_t s_sclIomux;
static uint32_t s_sdaIomux;

/* SDA 方向追踪 */
static uint8_t s_sdaIsInput;

/* 延时预计算值 */
static uint8_t s_d1us, s_d2us, s_d4us, s_d5us;

/* 延时关闭标志 */
static uint8_t s_delayOff;

/* ===================== 内部辅助 ===================== */

static void calc_delays(uint32_t speedKhz)
{
	uint8_t mult;

	switch (speedKhz)
	{
	case 400U: mult = 1U; break;
	case 200U: mult = 3U; break;
	case 50U:  mult = 10U; break;
	default:   mult = 5U; break; /* 100K */
	}

#define I2C_DIV 5U
	s_d1us = (uint8_t)((((uint16_t)1U * mult) + I2C_DIV - 1U) / I2C_DIV);
	s_d2us = (uint8_t)((((uint16_t)2U * mult) + I2C_DIV - 1U) / I2C_DIV);
	s_d4us = (uint8_t)((((uint16_t)4U * mult) + I2C_DIV - 1U) / I2C_DIV);
	s_d5us = (uint8_t)((((uint16_t)5U * mult) + I2C_DIV - 1U) / I2C_DIV);
	if (s_d1us == 0U) { s_d1us = 1U; }
	if (s_d2us == 0U) { s_d2us = 1U; }
	if (s_d4us == 0U) { s_d4us = 1U; }
	if (s_d5us == 0U) { s_d5us = 1U; }
#undef I2C_DIV
}

static void g3507_pin_init_output(GPIO_Regs *reg, uint32_t pin, uint32_t iomux)
{
	if (!DL_GPIO_isPowerEnabled(reg))
	{
		DL_GPIO_reset(reg);
		DL_GPIO_enablePower(reg);
		while (!DL_GPIO_isPowerEnabled(reg)) { }
	}
	reg->DOUTSET31_0 = pin;
	IOMUX->SECCFG.PINCM[iomux] =
		IOMUX_PINCM_PC_CONNECTED |
		((uint32_t)0x00000001U) |
		IOMUX_PINCM_INV_DISABLE |
		IOMUX_PINCM_PIPU_DISABLE |
		IOMUX_PINCM_PIPD_DISABLE |
		IOMUX_PINCM_DRV_DRVVAL0 |
		IOMUX_PINCM_HIZ1_DISABLE;
	reg->DOESET31_0 = pin;
}

/* ===================== HAL 接口实现 ===================== */

void soft_i2c_hal_init(void *sclPort, uint32_t sclPin, uint32_t sclIomux,
                       void *sdaPort, uint32_t sdaPin, uint32_t sdaIomux)
{
	s_sclReg = (GPIO_Regs *)sclPort;
	s_sdaReg = (GPIO_Regs *)sdaPort;
	s_sclPin = sclPin;
	s_sdaPin = sdaPin;
	s_sclIomux = sclIomux;
	s_sdaIomux = sdaIomux;

	/* 初始化 GPIO */
	g3507_pin_init_output(s_sclReg, sclPin, sclIomux);
	if ((uintptr_t)sdaPort != (uintptr_t)sclPort)
	{
		g3507_pin_init_output(s_sdaReg, sdaPin, sdaIomux);
	}

	s_sdaIsInput = 0U;
}

void soft_i2c_hal_w_scl(uint8_t bit)
{
	if (bit != 0U)
	{
		s_sclReg->DOUTSET31_0 = s_sclPin;
	}
	else
	{
		s_sclReg->DOUTCLR31_0 = s_sclPin;
	}
	soft_i2c_hal_delay_us(5U);
}

void soft_i2c_hal_w_sda(uint8_t bit)
{
	if (s_sdaIsInput != 0U)
	{
		soft_i2c_hal_set_sda_output();
	}

	if (bit != 0U)
	{
		s_sdaReg->DOUTSET31_0 = s_sdaPin;
	}
	else
	{
		s_sdaReg->DOUTCLR31_0 = s_sdaPin;
	}
	soft_i2c_hal_delay_us(5U);
}

uint8_t soft_i2c_hal_r_sda(void)
{
	soft_i2c_hal_delay_us(5U);
	return ((s_sdaReg->DIN31_0 & s_sdaPin) != 0U) ? 1U : 0U;
}

void soft_i2c_hal_set_sda_input(void)
{
	if (s_sdaIsInput != 0U) { return; }
	s_sdaReg->DOECLR31_0 = s_sdaPin;
	IOMUX->SECCFG.PINCM[s_sdaIomux] =
		IOMUX_PINCM_INENA_ENABLE |
		IOMUX_PINCM_PC_CONNECTED |
		((uint32_t)0x00000001U) |
		IOMUX_PINCM_INV_DISABLE |
		IOMUX_PINCM_HYSTEN_DISABLE |
		IOMUX_PINCM_WUEN_DISABLE |
		IOMUX_PINCM_PIPU_ENABLE |
		IOMUX_PINCM_PIPD_DISABLE;
	s_sdaIsInput = 1U;
}

void soft_i2c_hal_set_sda_output(void)
{
	s_sdaReg->DOUTCLR31_0 = s_sdaPin;
	IOMUX->SECCFG.PINCM[s_sdaIomux] =
		IOMUX_PINCM_PC_CONNECTED |
		((uint32_t)0x00000001U) |
		IOMUX_PINCM_INV_DISABLE |
		IOMUX_PINCM_PIPU_DISABLE |
		IOMUX_PINCM_PIPD_DISABLE |
		IOMUX_PINCM_DRV_DRVVAL0 |
		IOMUX_PINCM_HIZ1_DISABLE;
	s_sdaReg->DOESET31_0 = s_sdaPin;
	s_sdaIsInput = 0U;
}

void soft_i2c_hal_delay_us(uint32_t us)
{
	uint8_t d;

	if (s_delayOff != 0U)
	{
		return;
	}

	if (us == 5U)
	{
		d = s_d5us;
	}
	else if (us == 4U)
	{
		d = s_d4us;
	}
	else if (us == 2U)
	{
		d = s_d2us;
	}
	else
	{
		d = s_d1us;
	}
	Delay_us((uint32_t)d);
}

void soft_i2c_hal_set_speed(uint32_t speedKhz)
{
	calc_delays(speedKhz);
}

void soft_i2c_hal_delay_off(void)
{
	s_delayOff = 1U;
}

void soft_i2c_hal_delay_on(void)
{
	s_delayOff = 0U;
}

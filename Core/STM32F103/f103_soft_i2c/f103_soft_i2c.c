#include "f103_soft_i2c.h"

#include "f103_gpio.h"
#include "Delay.h"

/* ===================== 模块状态变量 ===================== */

/* F103 寄存器缓存 */
static F103_GPIO_Regs_t *s_sclReg;
static F103_GPIO_Regs_t *s_sdaReg;
static uint32_t s_sclPin;
static uint32_t s_sdaPin;

/* SDA CRL/CRH 预计算值 */
static volatile uint32_t *s_sdaCrReg;
static uint32_t s_sdaCrMask;
static uint32_t s_sdaCrOutValue;
static uint32_t s_sdaCrInValue;

/* SDA 方向追踪 */
static uint8_t s_sdaIsInput;

/* 延时预计算值 */
static uint8_t s_d1us, s_d2us, s_d4us, s_d5us;

/* 延时关闭标志 */
static uint8_t s_delayOff;

/* ===================== 内部辅助 ===================== */

static uint32_t pin_index(uint32_t pin)
{
	uint32_t idx = 0U;
	while ((pin & 1U) == 0U)
	{
		pin >>= 1U;
		idx++;
	}
	return idx;
}

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

static uint32_t f103_pin_init_output(F103_GPIO_Regs_t *reg, uint32_t pin)
{
	uint32_t idx = pin_index(pin);
	uint32_t sh = (idx & 0x7U) * 4U;
	F103_GPIO_EnablePortClock(reg);
	if (idx < 8U)
	{
		reg->CRL = (reg->CRL & ~(0xFUL << sh)) | (0x2UL << sh);
	}
	else
	{
		reg->CRH = (reg->CRH & ~(0xFUL << sh)) | (0x2UL << sh);
	}
	reg->BSRR = pin;
	return idx;
}

/* ===================== HAL 接口实现 ===================== */

void soft_i2c_hal_init(void *sclPort, uint32_t sclPin, uint32_t sclIomux,
                       void *sdaPort, uint32_t sdaPin, uint32_t sdaIomux)
{
	uint32_t idx;
	uint32_t sh;

	(void)sclIomux;
	(void)sdaIomux;

	s_sclReg = (F103_GPIO_Regs_t *)sclPort;
	s_sdaReg = (F103_GPIO_Regs_t *)sdaPort;
	s_sclPin = sclPin;
	s_sdaPin = sdaPin;

	/* 初始化 GPIO */
	F103_GPIO_EnablePortClock(sclPort);
	if ((uintptr_t)sdaPort != (uintptr_t)sclPort)
	{
		F103_GPIO_EnablePortClock(sdaPort);
	}
	f103_pin_init_output(s_sclReg, sclPin);
	f103_pin_init_output(s_sdaReg, sdaPin);

	/* SDA CRL/CRH 预计算 */
	idx = pin_index(sdaPin);
	sh = (idx & 0x7U) * 4U;
	s_sdaCrMask = ~(0xFUL << sh);
	s_sdaCrOutValue = 0x2UL << sh;
	s_sdaCrInValue = 0x8UL << sh;
	if (idx < 8U)
	{
		s_sdaCrReg = &s_sdaReg->CRL;
	}
	else
	{
		s_sdaCrReg = &s_sdaReg->CRH;
	}

	s_sdaIsInput = 0U;
}

void soft_i2c_hal_w_scl(uint8_t bit)
{
	if (bit != 0U)
	{
		s_sclReg->BSRR = s_sclPin;
	}
	else
	{
		s_sclReg->BRR = s_sclPin;
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
		s_sdaReg->BSRR = s_sdaPin;
	}
	else
	{
		s_sdaReg->BRR = s_sdaPin;
	}
	soft_i2c_hal_delay_us(5U);
}

uint8_t soft_i2c_hal_r_sda(void)
{
	soft_i2c_hal_delay_us(5U);
	return ((s_sdaReg->IDR & s_sdaPin) != 0U) ? 1U : 0U;
}

void soft_i2c_hal_set_sda_input(void)
{
	if (s_sdaIsInput != 0U) { return; }
	*s_sdaCrReg = (*s_sdaCrReg & s_sdaCrMask) | s_sdaCrInValue;
	s_sdaReg->ODR |= s_sdaPin;
	s_sdaIsInput = 1U;
}

void soft_i2c_hal_set_sda_output(void)
{
	*s_sdaCrReg = (*s_sdaCrReg & s_sdaCrMask) | s_sdaCrOutValue;
	s_sdaReg->BRR = s_sdaPin;
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

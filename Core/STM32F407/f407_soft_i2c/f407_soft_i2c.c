#include "f407_soft_i2c.h"

#include "f407_gpio.h"
#include "Delay.h"

/* ===================== 模块状态变量 ===================== */

/* F407 寄存器缓存 */
static F407_GPIO_Regs_t *s_sclReg;
static F407_GPIO_Regs_t *s_sdaReg;
static uint32_t s_sclPin;
static uint32_t s_sdaPin;

/* SDA MODER/PUPDR 预计算 */
static uint32_t s_sdaShift2;
static uint32_t s_sdaModerMask;
static uint32_t s_sdaPupdrMask;

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

static void f407_pin_init_output(F407_GPIO_Regs_t *reg, uint32_t pin)
{
	uint32_t idx = pin_index(pin);
	uint32_t sh = idx * 2U;
	F407_GPIO_EnablePortClock(reg);
	reg->MODER = (reg->MODER & ~(0x3UL << sh)) | (0x1UL << sh);
	reg->OTYPER &= ~((uint32_t)1U << idx);
	reg->OSPEEDR &= ~(0x3UL << sh);
	reg->PUPDR &= ~(0x3UL << sh);
	reg->BSRR = pin;
}

/* ===================== HAL 接口实现 ===================== */

void soft_i2c_hal_init(void *sclPort, uint32_t sclPin, uint32_t sclIomux,
                       void *sdaPort, uint32_t sdaPin, uint32_t sdaIomux)
{
	uint32_t idx;

	(void)sclIomux;
	(void)sdaIomux;

	s_sclReg = (F407_GPIO_Regs_t *)sclPort;
	s_sdaReg = (F407_GPIO_Regs_t *)sdaPort;
	s_sclPin = sclPin;
	s_sdaPin = sdaPin;

	/* 初始化 GPIO */
	f407_pin_init_output(s_sclReg, sclPin);
	if ((uintptr_t)sdaPort != (uintptr_t)sclPort)
	{
		f407_pin_init_output(s_sdaReg, sdaPin);
	}

	/* SDA MODER/PUPDR 预计算 */
	idx = pin_index(sdaPin);
	s_sdaShift2 = idx * 2U;
	s_sdaModerMask = ~(0x3UL << s_sdaShift2);
	s_sdaPupdrMask = ~(0x3UL << s_sdaShift2);

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
		s_sclReg->BSRR = ((uint32_t)s_sclPin << 16U);
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
		s_sdaReg->BSRR = ((uint32_t)s_sdaPin << 16U);
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
	s_sdaReg->MODER = (s_sdaReg->MODER & s_sdaModerMask);
	s_sdaReg->PUPDR = (s_sdaReg->PUPDR & s_sdaPupdrMask) | (1UL << s_sdaShift2);
	s_sdaIsInput = 1U;
}

void soft_i2c_hal_set_sda_output(void)
{
	s_sdaReg->MODER = (s_sdaReg->MODER & s_sdaModerMask) | (1UL << s_sdaShift2);
	s_sdaReg->PUPDR = (s_sdaReg->PUPDR & s_sdaPupdrMask);
	s_sdaReg->BSRR = ((uint32_t)s_sdaPin << 16U);
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

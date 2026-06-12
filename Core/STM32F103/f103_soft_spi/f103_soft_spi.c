#include "f103_soft_spi.h"

#include "f103_gpio.h"
#include "Delay.h"

/* ===================== 模块状态变量 ===================== */

/* F103 寄存器缓存 */
static F103_GPIO_Regs_t *s_csReg;
static F103_GPIO_Regs_t *s_sckReg;
static F103_GPIO_Regs_t *s_mosiReg;
static F103_GPIO_Regs_t *s_misoReg;
static uint32_t s_csPin;
static uint32_t s_sckPin;
static uint32_t s_mosiPin;
static uint32_t s_misoPin;

/* 预计算的基础延时 (us) */
static uint8_t s_spiDelayUs;

/* 延时关闭标志 */
static uint8_t s_spiDelayOff;

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

static void f103_spi_init_output(F103_GPIO_Regs_t *reg, uint32_t pin)
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
}

static void f103_spi_init_input(F103_GPIO_Regs_t *reg, uint32_t pin)
{
	uint32_t idx = pin_index(pin);
	uint32_t sh = (idx & 0x7U) * 4U;
	F103_GPIO_EnablePortClock(reg);
	if (idx < 8U)
	{
		reg->CRL = (reg->CRL & ~(0xFUL << sh)) | (0x4UL << sh);
	}
	else
	{
		reg->CRH = (reg->CRH & ~(0xFUL << sh)) | (0x4UL << sh);
	}
}

/* ===================== HAL 接口实现 ===================== */

void soft_spi_hal_init(void *csPort, uint32_t csPin, uint32_t csIomux,
                       void *sckPort, uint32_t sckPin, uint32_t sckIomux,
                       void *mosiPort, uint32_t mosiPin, uint32_t mosiIomux,
                       void *misoPort, uint32_t misoPin, uint32_t misoIomux)
{
	(void)csIomux;
	(void)sckIomux;
	(void)mosiIomux;
	(void)misoIomux;

	s_csReg   = (F103_GPIO_Regs_t *)csPort;
	s_sckReg  = (F103_GPIO_Regs_t *)sckPort;
	s_mosiReg = (F103_GPIO_Regs_t *)mosiPort;
	s_misoReg = (F103_GPIO_Regs_t *)misoPort;
	s_csPin   = csPin;
	s_sckPin  = sckPin;
	s_mosiPin = mosiPin;
	s_misoPin = misoPin;

	f103_spi_init_output(s_csReg, csPin);
	f103_spi_init_output(s_sckReg, sckPin);
	f103_spi_init_output(s_mosiReg, mosiPin);
	f103_spi_init_input(s_misoReg, misoPin);
}

void soft_spi_hal_w_cs(uint8_t bit)
{
	if (bit != 0U)
	{
		s_csReg->BSRR = s_csPin;
	}
	else
	{
		s_csReg->BRR = s_csPin;
	}
}

void soft_spi_hal_w_sck(uint8_t bit)
{
	if (bit != 0U)
	{
		s_sckReg->BSRR = s_sckPin;
	}
	else
	{
		s_sckReg->BRR = s_sckPin;
	}
}

void soft_spi_hal_w_mosi(uint8_t bit)
{
	if (bit != 0U)
	{
		s_mosiReg->BSRR = s_mosiPin;
	}
	else
	{
		s_mosiReg->BRR = s_mosiPin;
	}
}

uint8_t soft_spi_hal_r_miso(void)
{
	return ((s_misoReg->IDR & s_misoPin) != 0U) ? 1U : 0U;
}

void soft_spi_hal_delay_us(uint32_t us)
{
	if (s_spiDelayOff != 0U || s_spiDelayUs == 0U)
	{
		(void)us;
		return;
	}
	Delay_us((uint32_t)s_spiDelayUs);
}

void soft_spi_hal_set_speed(uint32_t speedKhz)
{
	switch (speedKhz)
	{
	case 5000U: s_spiDelayUs = 0U; break;
	case 2000U: s_spiDelayUs = 0U; break;
	case 1000U: s_spiDelayUs = 1U; break;
	case 250U:  s_spiDelayUs = 4U; break;
	default:    s_spiDelayUs = 2U; break; /* 500K */
	}
}

void soft_spi_hal_delay_off(void)
{
	s_spiDelayOff = 1U;
}

void soft_spi_hal_delay_on(void)
{
	s_spiDelayOff = 0U;
}

#include "G3507_soft_spi.h"

#include "G3507_gpio.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "Delay.h"

/* ===================== 模块状态变量 ===================== */

/* G3507 寄存器缓存 */
static GPIO_Regs *s_csReg;
static GPIO_Regs *s_sckReg;
static GPIO_Regs *s_mosiReg;
static GPIO_Regs *s_misoReg;
static uint32_t s_csPin;
static uint32_t s_sckPin;
static uint32_t s_mosiPin;
static uint32_t s_misoPin;

/* 预计算的基础延时 (us) */
static uint8_t s_spiDelayUs;

/* 延时关闭标志 */
static uint8_t s_spiDelayOff;

/* ===================== 内部辅助 ===================== */

static void g3507_spi_init_output(GPIO_Regs *reg, uint32_t pin, uint32_t iomux)
{
	if (!DL_GPIO_isPowerEnabled(reg))
	{
		DL_GPIO_reset(reg);
		DL_GPIO_enablePower(reg);
		while (!DL_GPIO_isPowerEnabled(reg)) { }
	}
	reg->DOUTCLR31_0 = pin;
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

static void g3507_spi_init_input(GPIO_Regs *reg, uint32_t pin, uint32_t iomux)
{
	if (!DL_GPIO_isPowerEnabled(reg))
	{
		DL_GPIO_reset(reg);
		DL_GPIO_enablePower(reg);
		while (!DL_GPIO_isPowerEnabled(reg)) { }
	}
	reg->DOECLR31_0 = pin;
	IOMUX->SECCFG.PINCM[iomux] =
		IOMUX_PINCM_INENA_ENABLE |
		IOMUX_PINCM_PC_CONNECTED |
		((uint32_t)0x00000001U) |
		IOMUX_PINCM_INV_DISABLE |
		IOMUX_PINCM_HYSTEN_DISABLE |
		IOMUX_PINCM_WUEN_DISABLE |
		IOMUX_PINCM_PIPU_ENABLE |
		IOMUX_PINCM_PIPD_DISABLE;
}

/* ===================== HAL 接口实现 ===================== */

void soft_spi_hal_init(void *csPort, uint32_t csPin, uint32_t csIomux,
                       void *sckPort, uint32_t sckPin, uint32_t sckIomux,
                       void *mosiPort, uint32_t mosiPin, uint32_t mosiIomux,
                       void *misoPort, uint32_t misoPin, uint32_t misoIomux)
{
	s_csReg   = (GPIO_Regs *)csPort;
	s_sckReg  = (GPIO_Regs *)sckPort;
	s_mosiReg = (GPIO_Regs *)mosiPort;
	s_misoReg = (GPIO_Regs *)misoPort;
	s_csPin   = csPin;
	s_sckPin  = sckPin;
	s_mosiPin = mosiPin;
	s_misoPin = misoPin;

	g3507_spi_init_output(s_csReg, csPin, csIomux);
	g3507_spi_init_output(s_sckReg, sckPin, sckIomux);
	g3507_spi_init_output(s_mosiReg, mosiPin, mosiIomux);
	g3507_spi_init_input(s_misoReg, misoPin, misoIomux);
}

void soft_spi_hal_w_cs(uint8_t bit)
{
	if (bit != 0U)
	{
		s_csReg->DOUTSET31_0 = s_csPin;
	}
	else
	{
		s_csReg->DOUTCLR31_0 = s_csPin;
	}
}

void soft_spi_hal_w_sck(uint8_t bit)
{
	if (bit != 0U)
	{
		s_sckReg->DOUTSET31_0 = s_sckPin;
	}
	else
	{
		s_sckReg->DOUTCLR31_0 = s_sckPin;
	}
}

void soft_spi_hal_w_mosi(uint8_t bit)
{
	if (bit != 0U)
	{
		s_mosiReg->DOUTSET31_0 = s_mosiPin;
	}
	else
	{
		s_mosiReg->DOUTCLR31_0 = s_mosiPin;
	}
}

uint8_t soft_spi_hal_r_miso(void)
{
	return ((s_misoReg->DIN31_0 & s_misoPin) != 0U) ? 1U : 0U;
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

#include "G3507_gpio.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "G3507_pinmux.h" // 引入查表参考

/*
 * G3507_gpio.c
 * 通过 pin mask + port 在本地表中查找 IOMUX PINCM。
 */

static uint32_t G3507_PinToIndex(uint32_t pin)
{
	uint32_t index;

	for (index = 0U; index < 32U; ++index)
	{
		if (pin == (1UL << index))
		{
			return index;
		}
	}

	return 0xFFFFFFFFUL;
}

typedef enum
{
	G3507_GPIO_MODE_UNKNOWN = 0,
	G3507_GPIO_MODE_OUTPUT,
	G3507_GPIO_MODE_INPUT,
	G3507_GPIO_MODE_INPUT_PULLUP
} G3507_GpioMode_t;

static uint8_t s_gpioPortPoweredA;
static uint8_t s_gpioPortPoweredB;
static uint8_t s_gpioModeA[32U];
static uint8_t s_gpioModeB[32U];

static uint8_t *G3507_GetPortPowerStatePtr(GPIO_Regs *gpioPort)
{
	if (gpioPort == GPIOA)
	{
		return &s_gpioPortPoweredA;
	}

	if (gpioPort == GPIOB)
	{
		return &s_gpioPortPoweredB;
	}

	return 0;
}

static uint8_t *G3507_GetPinModePtr(GPIO_Regs *gpioPort, uint32_t pinIndex)
{
	if (pinIndex >= 32U)
	{
		return 0;
	}

	if (gpioPort == GPIOA)
	{
		return &s_gpioModeA[pinIndex];
	}

	if (gpioPort == GPIOB)
	{
		return &s_gpioModeB[pinIndex];
	}

	return 0;
}

static void G3507_ResetPortModeCache(GPIO_Regs *gpioPort)
{
	uint32_t i;
	uint8_t *modeArray;

	if (gpioPort == GPIOA)
	{
		modeArray = s_gpioModeA;
	}
	else if (gpioPort == GPIOB)
	{
		modeArray = s_gpioModeB;
	}
	else
	{
		return;
	}

	for (i = 0U; i < 32U; ++i)
	{
		modeArray[i] = (uint8_t)G3507_GPIO_MODE_UNKNOWN;
	}
}

static void G3507_IomuxSetDigitalOutput(uint32_t iomux)
{
	/* 等价于 DL_GPIO_initDigitalOutputFeatures(..., RESISTOR_NONE)。 */
	IOMUX->SECCFG.PINCM[iomux] = IOMUX_PINCM_PC_CONNECTED |
		((uint32_t)0x00000001U) |
		IOMUX_PINCM_INV_DISABLE |
		IOMUX_PINCM_PIPU_DISABLE |
		IOMUX_PINCM_PIPD_DISABLE |
		IOMUX_PINCM_DRV_DRVVAL0 |
		IOMUX_PINCM_HIZ1_DISABLE;
}

static void G3507_IomuxSetDigitalInput(uint32_t iomux, uint8_t pullUp)
{
	uint32_t regValue;

	/* 等价于 DL_GPIO_initDigitalInputFeatures(..., WAKEUP_DISABLE)。 */
	regValue = IOMUX_PINCM_INENA_ENABLE |
		IOMUX_PINCM_PC_CONNECTED |
		((uint32_t)0x00000001U) |
		IOMUX_PINCM_INV_DISABLE |
		IOMUX_PINCM_HYSTEN_DISABLE |
		IOMUX_PINCM_WUEN_DISABLE;

	if (pullUp != 0U)
	{
		regValue |= IOMUX_PINCM_PIPU_ENABLE;
		regValue |= IOMUX_PINCM_PIPD_DISABLE;
	}
	else
	{
		regValue |= IOMUX_PINCM_PIPU_DISABLE;
		regValue |= IOMUX_PINCM_PIPD_DISABLE;
	}

	IOMUX->SECCFG.PINCM[iomux] = regValue;
}

static uint32_t G3507_GetPortAIomux(uint32_t pinIndex)
{
	static const uint32_t s_aIomux[] = {
		A0, A1, A2, A3, A4, A5, A6, A7,
		A8, A9, A10, A11, A12, A13, A14, A15,
		A16, A17, A18, A19, A20, A21, A22, A23,
		A24, A25, A26, A27, A28, A29, A30, A31
	};

	if (pinIndex < (sizeof(s_aIomux) / sizeof(s_aIomux[0])))
	{
		return s_aIomux[pinIndex];
	}

	return 0xFFFFFFFFUL;
}

static uint32_t G3507_GetPortBIomux(uint32_t pinIndex)
{
	static const uint32_t s_bIomux[] = {
		B0, B1, B2, B3, B4, B5, B6, B7,
		B8, B9, B10, B11, B12, B13, B14, B15,
		B16, B17, B18, B19, B20, B21, B22, B23,
		B24, B25, B26, B27
	};

	if (pinIndex < (sizeof(s_bIomux) / sizeof(s_bIomux[0])))
	{
		return s_bIomux[pinIndex];
	}

	return 0xFFFFFFFFUL;
}

/* 根据 port+pin 获取 IOMUX */
uint32_t G3507_GetIomux(void *port, uint32_t pin)
{
	uint32_t pinIndex;

	pinIndex = G3507_PinToIndex(pin);
	if (pinIndex == 0xFFFFFFFFUL)
	{
		return 0xFFFFFFFFUL;
	}

    switch ((uintptr_t)port)
    {
        case (uintptr_t)GPIOA:
			return G3507_GetPortAIomux(pinIndex);
        case (uintptr_t)GPIOB:
			return G3507_GetPortBIomux(pinIndex);
        default:
            return 0xFFFFFFFFUL;
    }
}

/* 确保 GPIO 端口电源已开启。 */
static void G3507_GPIO_EnsurePower(GPIO_Regs *gpioPort)
{
	uint8_t *powerState;

	powerState = G3507_GetPortPowerStatePtr(gpioPort);
	if ((powerState != 0) && (*powerState != 0U))
	{
		return;
	}

	if (!DL_GPIO_isPowerEnabled(gpioPort))
	{
		DL_GPIO_reset(gpioPort);
		G3507_ResetPortModeCache(gpioPort);
		DL_GPIO_enablePower(gpioPort);
		while (!DL_GPIO_isPowerEnabled(gpioPort))
		{
		}
	}

	if (powerState != 0)
	{
		*powerState = 1U;
	}
}

/*
 * G3507 GPIO 输出初始化完整序列：
 * 1) 确保端口电源开启
 * 2) 默认输出低电平
 * 3) 配置 IOMUX 为数字输出
 * 4) 使能输出模式
 */
void G3507_GPIO_InitOutput(void *port, uint32_t pin)
{
	GPIO_Regs *gpioPort;
	uint32_t iomux;
	uint32_t pinIndex;
	uint8_t *modePtr;

	if (port == 0)
	{
		return;
	}

	gpioPort = (GPIO_Regs *)port;
	iomux    = G3507_GetIomux(gpioPort, pin);
	if (iomux == 0xFFFFFFFFUL)
	{
		return;
	}

	pinIndex = G3507_PinToIndex(pin);
	if (pinIndex == 0xFFFFFFFFUL)
	{
		return;
	}

	modePtr = G3507_GetPinModePtr(gpioPort, pinIndex);
	if ((modePtr != 0) && (*modePtr == (uint8_t)G3507_GPIO_MODE_OUTPUT))
	{
		gpioPort->DOESET31_0 = pin;
		return;
	}

	G3507_GPIO_EnsurePower(gpioPort);
	gpioPort->DOUTCLR31_0 = pin;
	G3507_IomuxSetDigitalOutput(iomux);
	gpioPort->DOESET31_0 = pin;

	if (modePtr != 0)
	{
		*modePtr = (uint8_t)G3507_GPIO_MODE_OUTPUT;
	}
}

/* GPIO 浮空输入初始化。 */
void G3507_GPIO_InitInput(void *port, uint32_t pin)
{
	GPIO_Regs *gpioPort;
	uint32_t iomux;
	uint32_t pinIndex;
	uint8_t *modePtr;

	if (port == 0)
	{
		return;
	}

	gpioPort = (GPIO_Regs *)port;
	iomux    = G3507_GetIomux(gpioPort, pin);
	if (iomux == 0xFFFFFFFFUL)
	{
		return;
	}

	pinIndex = G3507_PinToIndex(pin);
	if (pinIndex == 0xFFFFFFFFUL)
	{
		return;
	}

	modePtr = G3507_GetPinModePtr(gpioPort, pinIndex);
	if ((modePtr != 0) && (*modePtr == (uint8_t)G3507_GPIO_MODE_INPUT))
	{
		return;
	}

	G3507_GPIO_EnsurePower(gpioPort);
	gpioPort->DOECLR31_0 = pin;
	G3507_IomuxSetDigitalInput(iomux, 0U);

	if (modePtr != 0)
	{
		*modePtr = (uint8_t)G3507_GPIO_MODE_INPUT;
	}
}

/* GPIO 上拉输入初始化。 */
void G3507_GPIO_InitInputPullUp(void *port, uint32_t pin)
{
	GPIO_Regs *gpioPort;
	uint32_t iomux;
	uint32_t pinIndex;
	uint8_t *modePtr;

	if (port == 0)
	{
		return;
	}

	gpioPort = (GPIO_Regs *)port;
	iomux    = G3507_GetIomux(gpioPort, pin);
	if (iomux == 0xFFFFFFFFUL)
	{
		return;
	}

	pinIndex = G3507_PinToIndex(pin);
	if (pinIndex == 0xFFFFFFFFUL)
	{
		return;
	}

	modePtr = G3507_GetPinModePtr(gpioPort, pinIndex);
	if ((modePtr != 0) && (*modePtr == (uint8_t)G3507_GPIO_MODE_INPUT_PULLUP))
	{
		return;
	}

	G3507_GPIO_EnsurePower(gpioPort);
	gpioPort->DOECLR31_0 = pin;
	G3507_IomuxSetDigitalInput(iomux, 1U);

	if (modePtr != 0)
	{
		*modePtr = (uint8_t)G3507_GPIO_MODE_INPUT_PULLUP;
	}
}

/* GPIO 写电平：level 非 0 读高电平，0 读低电平。 */
void G3507_GPIO_Write(void *port, uint32_t pin, uint8_t level)
{
	GPIO_Regs *gpioPort;

	if (port == 0)
	{
		return;
	}

	gpioPort = (GPIO_Regs *)port;

	if (level != 0U)
	{
		gpioPort->DOUTSET31_0 = pin;
	}
	else
	{
		gpioPort->DOUTCLR31_0 = pin;
	}
}

/* GPIO 读电平：返回 1/0。 */
uint8_t G3507_GPIO_Read(void *port, uint32_t pin)
{
	GPIO_Regs *gpioPort;

	if ((port == 0) || (pin == 0U))
	{
		return 0U;
	}

	gpioPort = (GPIO_Regs *)port;

	return ((gpioPort->DIN31_0 & pin) != 0U) ? 1U : 0U;
}

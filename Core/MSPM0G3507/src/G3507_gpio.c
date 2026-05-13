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
	if (!DL_GPIO_isPowerEnabled(gpioPort))
	{
		DL_GPIO_reset(gpioPort);
		DL_GPIO_enablePower(gpioPort);
		while (!DL_GPIO_isPowerEnabled(gpioPort))
		{
		}
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
	uint32_t   iomux;

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

	G3507_GPIO_EnsurePower(gpioPort);
	DL_GPIO_clearPins(gpioPort, pin);
	DL_GPIO_initDigitalOutputFeatures((uint32_t)iomux,
		DL_GPIO_INVERSION_DISABLE,
		DL_GPIO_RESISTOR_NONE,
		DL_GPIO_DRIVE_STRENGTH_LOW,
		DL_GPIO_HIZ_DISABLE);
	DL_GPIO_enableOutput(gpioPort, pin);
}

/* GPIO 浮空输入初始化。 */
void G3507_GPIO_InitInput(void *port, uint32_t pin)
{
	GPIO_Regs *gpioPort;
	uint32_t   iomux;

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

	G3507_GPIO_EnsurePower(gpioPort);
	/* 切输入前先关闭输出驱动，避免保留上一次输出状态。 */
	DL_GPIO_disableOutput(gpioPort, pin);
	DL_GPIO_initDigitalInputFeatures((uint32_t)iomux,
		DL_GPIO_INVERSION_DISABLE,
		DL_GPIO_RESISTOR_NONE,
		DL_GPIO_HYSTERESIS_DISABLE,
		DL_GPIO_WAKEUP_DISABLE);
}

/* GPIO 上拉输入初始化。 */
void G3507_GPIO_InitInputPullUp(void *port, uint32_t pin)
{
	GPIO_Regs *gpioPort;
	uint32_t   iomux;

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

	G3507_GPIO_EnsurePower(gpioPort);
	/* 软件 I2C 释放高电平依赖输入上拉，必须先断开输出驱动。 */
	DL_GPIO_disableOutput(gpioPort, pin);
	DL_GPIO_initDigitalInputFeatures((uint32_t)iomux,
		DL_GPIO_INVERSION_DISABLE,
		DL_GPIO_RESISTOR_PULL_UP,
		DL_GPIO_HYSTERESIS_DISABLE,
		DL_GPIO_WAKEUP_DISABLE);
}

/* GPIO 写电平：level 非 0 读高电平，0 读低电平。 */
void G3507_GPIO_Write(void *port, uint32_t pin, uint8_t level)
{
	if (port == 0)
	{
		return;
	}

	if (level != 0U)
	{
		DL_GPIO_setPins((GPIO_Regs *)port, pin);
	}
	else
	{
		DL_GPIO_clearPins((GPIO_Regs *)port, pin);
	}
}

/* GPIO 读电平：返回 1/0。 */
uint8_t G3507_GPIO_Read(void *port, uint32_t pin)
{
	if ((port == 0) || (pin == 0U))
	{
		return 0U;
	}

	return (DL_GPIO_readPins((GPIO_Regs *)port, pin) != 0U) ? 1U : 0U;
}

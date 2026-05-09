#include "G3507_gpio.h"
#include "G3507_hw_config.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "G3507_pinmux.h" // 引入查表参考

/*
 * G3507_gpio.c
 * 删除查表逻辑，直接从 G3507_hw_config.h 获取 IOMUX。
 */

/* 根据 port+pin 获取 IOMUX */
uint32_t G3507_GetIomux(GPIO_Regs *port, uint32_t pin)
{
    switch ((uintptr_t)port)
    {
        case (uintptr_t)GPIOA:
            switch (pin)
            {
                case DL_GPIO_PIN_0: return A0;
                case DL_GPIO_PIN_1: return A1;
                case DL_GPIO_PIN_2: return A2;
                case DL_GPIO_PIN_3: return A3;
                case DL_GPIO_PIN_4: return A4;
                case DL_GPIO_PIN_5: return A5;
                case DL_GPIO_PIN_6: return A6;
                case DL_GPIO_PIN_7: return A7;
                case DL_GPIO_PIN_8: return A8;
                case DL_GPIO_PIN_9: return A9;
                case DL_GPIO_PIN_10: return A10;
                case DL_GPIO_PIN_11: return A11;
                case DL_GPIO_PIN_12: return A12;
                case DL_GPIO_PIN_13: return A13;
                case DL_GPIO_PIN_14: return A14;
                case DL_GPIO_PIN_15: return A15;
                default: return 0xFFFFFFFFUL;
            }
        case (uintptr_t)GPIOB:
            switch (pin)
            {
                case DL_GPIO_PIN_0: return B0;
                case DL_GPIO_PIN_1: return B1;
                case DL_GPIO_PIN_2: return B2;
                case DL_GPIO_PIN_3: return B3;
                case DL_GPIO_PIN_4: return B4;
                case DL_GPIO_PIN_5: return B5;
                case DL_GPIO_PIN_6: return B6;
                case DL_GPIO_PIN_7: return B7;
                case DL_GPIO_PIN_8: return B8;
                case DL_GPIO_PIN_9: return B9;
                case DL_GPIO_PIN_10: return B10;
                case DL_GPIO_PIN_11: return B11;
                case DL_GPIO_PIN_12: return B12;
                case DL_GPIO_PIN_13: return B13;
                case DL_GPIO_PIN_14: return B14;
                case DL_GPIO_PIN_15: return B15;
                default: return 0xFFFFFFFFUL;
            }
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

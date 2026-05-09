#include "f407_gpio.h"

/* 根据端口地址打开对应 AHB1 GPIO 时钟。 */
void F407_GPIO_EnablePortClock(void *port)
{
	F407_GPIO_Regs_t *gpioPort;

	if (port == 0)
	{
		return;
	}

	gpioPort = (F407_GPIO_Regs_t *)port;

	if (gpioPort == GPIOA)
	{
		F407_RCC->AHB1ENR |= (1UL << 0);
	}
	else if (gpioPort == GPIOB)
	{
		F407_RCC->AHB1ENR |= (1UL << 1);
	}
	else if (gpioPort == GPIOC)
	{
		F407_RCC->AHB1ENR |= (1UL << 2);
	}
	else if (gpioPort == GPIOD)
	{
		F407_RCC->AHB1ENR |= (1UL << 3);
	}
	else if (gpioPort == GPIOE)
	{
		F407_RCC->AHB1ENR |= (1UL << 4);
	}
	else if (gpioPort == GPIOF)
	{
		F407_RCC->AHB1ENR |= (1UL << 5);
	}
	else if (gpioPort == GPIOG)
	{
		F407_RCC->AHB1ENR |= (1UL << 6);
	}
	else if (gpioPort == GPIOH)
	{
		F407_RCC->AHB1ENR |= (1UL << 7);
	}
	else if (gpioPort == GPIOI)
	{
		F407_RCC->AHB1ENR |= (1UL << 8);
	}
	else
	{
		return;
	}
}

/*
 * 把单 bit 引脚掩码转换为引脚编号（0~15）。
 * 例如 GPIO_Pin_13 -> 13。
 */
uint32_t F407_GPIO_PinIndex(uint32_t pin)
{
	/* index: 遍历引脚编号。 */
	uint32_t index;

	for (index = 0U; index < 16U; ++index)
	{
		if (pin == (1UL << index))
		{
			return index;
		}
	}

	return 0xFFFFFFFFUL;
}

/*
 * GPIO 输出初始化：
 * 1) 校验端口和引脚
 * 2) 开启 GPIO 时钟
 * 3) 配置 MODER 为通用输出
 * 4) 配置 OTYPER 为推挽
 * 5) 配置 OSPEEDR 为低速
 */
void F407_GPIO_InitOutput(void *port, uint32_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F407_GPIO_Regs_t *gpioPort;
	/* pinIndex: 引脚编号 0~15。 */
	uint32_t pinIndex;
	/* shift: 当前引脚在 2bit 字段中的偏移。 */
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F407_GPIO_Regs_t *)port;
	pinIndex = F407_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	F407_GPIO_EnablePortClock(gpioPort);
	shift = pinIndex * 2U;

	/* MODER: 01 = 通用输出。 */
	gpioPort->MODER &= ~(0x3UL << shift);
	gpioPort->MODER |= (0x1UL << shift);

	/* OTYPER: 0 = 推挽。 */
	gpioPort->OTYPER &= ~(1UL << pinIndex);

	/* OSPEEDR: 00 = 低速。 */
	gpioPort->OSPEEDR &= ~(0x3UL << shift);

	/* PUPDR: 00 = 无上下拉。 */
	gpioPort->PUPDR &= ~(0x3UL << shift);
}

/* GPIO 输入初始化：配置为无上下拉输入。 */
void F407_GPIO_InitInput(void *port, uint32_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F407_GPIO_Regs_t *gpioPort;
	/* pinIndex: 引脚编号 0~15。 */
	uint32_t pinIndex;
	/* shift: 当前引脚在 2bit 字段中的偏移。 */
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F407_GPIO_Regs_t *)port;
	pinIndex = F407_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	F407_GPIO_EnablePortClock(gpioPort);
	shift = pinIndex * 2U;

	gpioPort->MODER &= ~(0x3UL << shift);
	gpioPort->PUPDR &= ~(0x3UL << shift);
}

/* GPIO 上拉输入初始化：配置为输入并打开内部上拉。 */
void F407_GPIO_InitInputPullUp(void *port, uint32_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F407_GPIO_Regs_t *gpioPort;
	/* pinIndex: 引脚编号 0~15。 */
	uint32_t pinIndex;
	/* shift: 当前引脚在 2bit 字段中的偏移。 */
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F407_GPIO_Regs_t *)port;
	pinIndex = F407_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	F407_GPIO_EnablePortClock(gpioPort);
	shift = pinIndex * 2U;

	gpioPort->MODER &= ~(0x3UL << shift);
	gpioPort->PUPDR &= ~(0x3UL << shift);
	gpioPort->PUPDR |= (0x1UL << shift);
}

/* GPIO 写电平接口：内部使用 BSRR 原子置位和复位。 */
void F407_GPIO_Write(void *port, uint32_t pin, uint8_t level)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F407_GPIO_Regs_t *gpioPort;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F407_GPIO_Regs_t *)port;
	if (level != 0U)
	{
		/* BSRR 低 16 位写 1：置位对应输出脚。 */
		gpioPort->BSRR = pin;
	}
	else
	{
		/* BSRR 高 16 位写 1：复位对应输出脚。 */
		gpioPort->BSRR = ((uint32_t)pin << 16U);
	}
}

/* GPIO 读电平接口：读取 IDR 对应位。 */
uint8_t F407_GPIO_Read(void *port, uint32_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F407_GPIO_Regs_t *gpioPort;

	if ((port == 0) || (pin == 0U))
	{
		return 0U;
	}

	gpioPort = (F407_GPIO_Regs_t *)port;
	return ((gpioPort->IDR & pin) != 0U) ? 1U : 0U;
}


#include "f103_gpio.h"

/* 根据端口地址打开对应 APB2 GPIO 时钟。 */
void F103_GPIO_EnablePortClock(void *port)
{
	F103_GPIO_Regs_t *gpioPort;

	if (port == 0)
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;

	/* F1 的 GPIO 时钟挂在 APB2，上电后必须先打开对应端口时钟。 */
	if (gpioPort == GPIOA)
	{
		F103_RCC->APB2ENR |= (1UL << 2);
	}
	else if (gpioPort == GPIOB)
	{
		F103_RCC->APB2ENR |= (1UL << 3);
	}
	else if (gpioPort == GPIOC)
	{
		F103_RCC->APB2ENR |= (1UL << 4);
	}
	else if (gpioPort == GPIOD)
	{
		F103_RCC->APB2ENR |= (1UL << 5);
	}
	else if (gpioPort == GPIOE)
	{
		F103_RCC->APB2ENR |= (1UL << 6);
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
uint32_t F103_GPIO_PinIndex(uint16_t pin)
{
	/* index: 遍历引脚编号。 */
	uint32_t index;

	for (index = 0U; index < 16U; ++index)
	{
		if (pin == (uint16_t)(1U << index))
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
 * 3) 配置 CRL/CRH 为 2MHz 推挽输出模式
 */
void F103_GPIO_InitOutput(void *port, uint16_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F103_GPIO_Regs_t *gpioPort;
	/* pinIndex: 引脚编号 0~15。 */
	uint32_t pinIndex;
	/* shift: 当前引脚在 CRL/CRH 的 4bit 偏移。 */
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	pinIndex = F103_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	/* GPIOx_CRL 配 0~7 号脚，GPIOx_CRH 配 8~15 号脚，每个脚占 4bit。 */
	F103_GPIO_EnablePortClock(gpioPort);
	shift = (pinIndex & 0x7U) * 4U;

	if (pinIndex < 8U)
	{
		gpioPort->CRL &= ~(0xFUL << shift);
		gpioPort->CRL |= (0x2UL << shift);
	}
	else
	{
		gpioPort->CRH &= ~(0xFUL << shift);
		gpioPort->CRH |= (0x2UL << shift);
	}
}

/* GPIO 输入初始化：配置为浮空输入。 */
void F103_GPIO_InitInput(void *port, uint16_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F103_GPIO_Regs_t *gpioPort;
	/* pinIndex: 引脚编号 0~15。 */
	uint32_t pinIndex;
	/* shift: 当前引脚在 CRL/CRH 的 4bit 偏移。 */
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	pinIndex = F103_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	F103_GPIO_EnablePortClock(gpioPort);
	shift = (pinIndex & 0x7U) * 4U;

	if (pinIndex < 8U)
	{
		gpioPort->CRL &= ~(0xFUL << shift);
		gpioPort->CRL |= (0x4UL << shift);
	}
	else
	{
		gpioPort->CRH &= ~(0xFUL << shift);
		gpioPort->CRH |= (0x4UL << shift);
	}
}

/* GPIO 上拉输入初始化：配置为输入上拉，并默认拉高对应引脚。 */
void F103_GPIO_InitInputPullUp(void *port, uint16_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F103_GPIO_Regs_t *gpioPort;
	/* pinIndex: 引脚编号 0~15。 */
	uint32_t pinIndex;
	/* shift: 当前引脚在 CRL/CRH 的 4bit 偏移。 */
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	pinIndex = F103_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	F103_GPIO_EnablePortClock(gpioPort);
	shift = (pinIndex & 0x7U) * 4U;

	if (pinIndex < 8U)
	{
		gpioPort->CRL &= ~(0xFUL << shift);
		gpioPort->CRL |= (0x8UL << shift);
	}
	else
	{
		gpioPort->CRH &= ~(0xFUL << shift);
		gpioPort->CRH |= (0x8UL << shift);
	}

	gpioPort->ODR |= pin;
}

/* GPIO 写电平接口：内部使用 BSRR/BRR 原子置位和复位。 */
void F103_GPIO_Write(void *port, uint16_t pin, uint8_t level)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F103_GPIO_Regs_t *gpioPort;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	if (level != 0U)
	{
		/* BSRR 写 1 置位对应输出脚。 */
		gpioPort->BSRR = pin;
	}
	else
	{
		/* BRR 写 1 复位对应输出脚。 */
		gpioPort->BRR = pin;
	}
}

/* GPIO 读电平接口：读取 IDR 对应位。 */
uint8_t F103_GPIO_Read(void *port, uint16_t pin)
{
	/* gpioPort: GPIO 寄存器映射地址。 */
	F103_GPIO_Regs_t *gpioPort;

	if ((port == 0) || (pin == 0U))
	{
		return 0U;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	return ((gpioPort->IDR & pin) != 0U) ? 1U : 0U;
}


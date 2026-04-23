#include "f103_pwm.h"

typedef struct
{
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t SMCR;
	volatile uint32_t DIER;
	volatile uint32_t SR;
	volatile uint32_t EGR;
	volatile uint32_t CCMR1;
	volatile uint32_t CCMR2;
	volatile uint32_t CCER;
	volatile uint32_t CNT;
	volatile uint32_t PSC;
	volatile uint32_t ARR;
	volatile uint32_t RESERVED0;
	volatile uint32_t CCR1;
	volatile uint32_t CCR2;
	volatile uint32_t CCR3;
	volatile uint32_t CCR4;
} F103_PWM_GenRegs_t;

typedef struct
{
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t SMCR;
	volatile uint32_t DIER;
	volatile uint32_t SR;
	volatile uint32_t EGR;
	volatile uint32_t CCMR1;
	volatile uint32_t CCMR2;
	volatile uint32_t CCER;
	volatile uint32_t CNT;
	volatile uint32_t PSC;
	volatile uint32_t ARR;
	volatile uint32_t RESERVED0;
	volatile uint32_t CCR1;
	volatile uint32_t CCR2;
	volatile uint32_t CCR3;
	volatile uint32_t CCR4;
	volatile uint32_t BDTR;
} F103_PWM_AdvRegs_t;

typedef struct
{
	volatile uint32_t CR;
	volatile uint32_t CFGR;
	volatile uint32_t CIR;
	volatile uint32_t APB2RSTR;
	volatile uint32_t APB1RSTR;
	volatile uint32_t AHBENR;
	volatile uint32_t APB2ENR;
	volatile uint32_t APB1ENR;
	volatile uint32_t BDCR;
	volatile uint32_t CSR;
} F103_PWM_RCC_Regs_t;

typedef struct
{
	void *regs;
	uint8_t advanced;
	uint32_t rccBit;
	uint8_t apb2;
} F103_PWM_Map_t;

#define F103_PWM_RCC_BASE    (0x40021000UL)
#define F103_PWM_TIM1_BASE   (0x40012C00UL)
#define F103_PWM_TIM2_BASE   (0x40000000UL)
#define F103_PWM_TIM3_BASE   (0x40000400UL)
#define F103_PWM_TIM4_BASE   (0x40000800UL)

#define F103_PWM_RCC         ((F103_PWM_RCC_Regs_t *)F103_PWM_RCC_BASE)

static uint32_t F103_PWM_GetPinIndex(uint16_t pin)
{
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

static void F103_PWM_EnableGpioClock(void *port)
{
	if (port == GPIOA)
	{
		F103_PWM_RCC->APB2ENR |= (1UL << 2);
	}
	else if (port == GPIOB)
	{
		F103_PWM_RCC->APB2ENR |= (1UL << 3);
	}
	else if (port == GPIOC)
	{
		F103_PWM_RCC->APB2ENR |= (1UL << 4);
	}
	else if (port == GPIOD)
	{
		F103_PWM_RCC->APB2ENR |= (1UL << 5);
	}
	else if (port == GPIOE)
	{
		F103_PWM_RCC->APB2ENR |= (1UL << 6);
	}
}

static F103_PWM_Map_t F103_PWM_GetMap(uint8_t timId)
{
	F103_PWM_Map_t map;

	map.regs = 0;
	map.advanced = 0U;
	map.rccBit = 0U;
	map.apb2 = 0U;

	switch (timId)
	{
	case 1U:
		map.regs = (F103_PWM_AdvRegs_t *)F103_PWM_TIM1_BASE;
		map.advanced = 1U;
		map.rccBit = 11U;
		map.apb2 = 1U;
		break;
	case 2U:
		map.regs = (F103_PWM_GenRegs_t *)F103_PWM_TIM2_BASE;
		map.rccBit = 0U;
		break;
	case 3U:
		map.regs = (F103_PWM_GenRegs_t *)F103_PWM_TIM3_BASE;
		map.rccBit = 1U;
		break;
	case 4U:
		map.regs = (F103_PWM_GenRegs_t *)F103_PWM_TIM4_BASE;
		map.rccBit = 2U;
		break;
	default:
		break;
	}

	return map;
}

static void F103_PWM_ResetTimer(void *regs, uint8_t advanced)
{
	if (advanced != 0U)
	{
		F103_PWM_AdvRegs_t *tim = (F103_PWM_AdvRegs_t *)regs;

		tim->CR1 = 0U;
		tim->CR2 = 0U;
		tim->SMCR = 0U;
		tim->DIER = 0U;
		tim->SR = 0U;
		tim->EGR = 0U;
		tim->CCMR1 = 0U;
		tim->CCMR2 = 0U;
		tim->CCER = 0U;
		tim->CNT = 0U;
		tim->PSC = 0U;
		tim->ARR = 0U;
		tim->BDTR = 0U;
	}
	else
	{
		F103_PWM_GenRegs_t *tim = (F103_PWM_GenRegs_t *)regs;

		tim->CR1 = 0U;
		tim->CR2 = 0U;
		tim->SMCR = 0U;
		tim->DIER = 0U;
		tim->SR = 0U;
		tim->EGR = 0U;
		tim->CCMR1 = 0U;
		tim->CCMR2 = 0U;
		tim->CCER = 0U;
		tim->CNT = 0U;
		tim->PSC = 0U;
		tim->ARR = 0U;
	}
}

static void F103_PWM_SetChannel(void *regs, uint8_t advanced, uint8_t channel, uint32_t compare)
{
	if (channel == 0U)
	{
		return;
	}

	if (advanced != 0U)
	{
		F103_PWM_AdvRegs_t *tim = (F103_PWM_AdvRegs_t *)regs;

		switch (channel)
		{
		case 1U:
			if ((tim->CCER & ((1UL << 0) | (1UL << 2) | (1UL << 3))) != ((1UL << 0) | (1UL << 2) | (1UL << 3)))
			{
				tim->CCMR1 &= ~((7UL << 4) | (1UL << 3));
				tim->CCMR1 |= ((6UL << 4) | (1UL << 3));
				tim->CCER &= ~(1UL << 1);
				/* TIM1_CH1N 默认在 PB13，使用 CC1NP=1 使互补脚高电平占空比同向。 */
				tim->CCER |= ((1UL << 0) | (1UL << 2) | (1UL << 3));
			}
			tim->CCR1 = compare;
			break;
		case 2U:
			if ((tim->CCER & ((1UL << 4) | (1UL << 6) | (1UL << 7))) != ((1UL << 4) | (1UL << 6) | (1UL << 7)))
			{
				tim->CCMR1 &= ~((7UL << 12) | (1UL << 11));
				tim->CCMR1 |= ((6UL << 12) | (1UL << 11));
				tim->CCER &= ~(1UL << 5);
				/* TIM1_CH2N 默认在 PB14，使用 CC2NP=1 使互补脚高电平占空比同向。 */
				tim->CCER |= ((1UL << 4) | (1UL << 6) | (1UL << 7));
			}
			tim->CCR2 = compare;
			break;
		case 3U:
			if ((tim->CCER & ((1UL << 8) | (1UL << 10) | (1UL << 11))) != ((1UL << 8) | (1UL << 10) | (1UL << 11)))
			{
				tim->CCMR2 &= ~((7UL << 4) | (1UL << 3));
				tim->CCMR2 |= ((6UL << 4) | (1UL << 3));
				tim->CCER &= ~(1UL << 9);
				/* TIM1_CH3N 默认在 PB15，使用 CC3NP=1 使互补脚高电平占空比同向。 */
				tim->CCER |= ((1UL << 8) | (1UL << 10) | (1UL << 11));
			}
			tim->CCR3 = compare;
			break;
		case 4U:
			if ((tim->CCER & (1UL << 12)) == 0U)
			{
				tim->CCMR2 &= ~((7UL << 12) | (1UL << 11));
				tim->CCMR2 |= ((6UL << 12) | (1UL << 11));
				tim->CCER &= ~(1UL << 13);
				tim->CCER |= (1UL << 12);
			}
			tim->CCR4 = compare;
			break;
		default:
			break;
		}
	}
	else
	{
		F103_PWM_GenRegs_t *tim = (F103_PWM_GenRegs_t *)regs;

		switch (channel)
		{
		case 1U:
			if ((tim->CCER & (1UL << 0)) == 0U)
			{
				tim->CCMR1 &= ~((7UL << 4) | (1UL << 3));
				tim->CCMR1 |= ((6UL << 4) | (1UL << 3));
				tim->CCER &= ~(1UL << 1);
				tim->CCER |= (1UL << 0);
			}
			tim->CCR1 = compare;
			break;
		case 2U:
			if ((tim->CCER & (1UL << 4)) == 0U)
			{
				tim->CCMR1 &= ~((7UL << 12) | (1UL << 11));
				tim->CCMR1 |= ((6UL << 12) | (1UL << 11));
				tim->CCER &= ~(1UL << 5);
				tim->CCER |= (1UL << 4);
			}
			tim->CCR2 = compare;
			break;
		case 3U:
			if ((tim->CCER & (1UL << 8)) == 0U)
			{
				tim->CCMR2 &= ~((7UL << 4) | (1UL << 3));
				tim->CCMR2 |= ((6UL << 4) | (1UL << 3));
				tim->CCER &= ~(1UL << 9);
				tim->CCER |= (1UL << 8);
			}
			tim->CCR3 = compare;
			break;
		case 4U:
			if ((tim->CCER & (1UL << 12)) == 0U)
			{
				tim->CCMR2 &= ~((7UL << 12) | (1UL << 11));
				tim->CCMR2 |= ((6UL << 12) | (1UL << 11));
				tim->CCER &= ~(1UL << 13);
				tim->CCER |= (1UL << 12);
			}
			tim->CCR4 = compare;
			break;
		default:
			break;
		}
	}
}

/* 配置指定引脚为 PWM 复用推挽输出。 */
void F103_PWM_ConfigPin(void *port, uint16_t pin)
{
	F103_GPIO_Regs_t *gpioPort;
	uint32_t pinIndex;
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	pinIndex = F103_PWM_GetPinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	F103_PWM_EnableGpioClock(port);
	shift = (pinIndex & 0x7U) * 4U;

	/* F1 PWM 输出使用复用推挽，50MHz 模式。 */
	if (pinIndex < 8U)
	{
		gpioPort->CRL &= ~(0xFUL << shift);
		gpioPort->CRL |= (0xBUL << shift);
	}
	else
	{
		gpioPort->CRH &= ~(0xFUL << shift);
		gpioPort->CRH |= (0xBUL << shift);
	}
}

/* 初始化定时器基础参数：PSC/ARR、更新事件、启动计数。 */
void F103_PWM_InitTimer(uint8_t timId, uint16_t arr, uint16_t psc)
{
	F103_PWM_Map_t map;

	map = F103_PWM_GetMap(timId);
	if (map.regs == 0)
	{
		return;
	}

	if (map.apb2 != 0U)
	{
		F103_PWM_RCC->APB2ENR |= (1UL << map.rccBit);
	}
	else
	{
		F103_PWM_RCC->APB1ENR |= (1UL << map.rccBit);
	}

	F103_PWM_ResetTimer(map.regs, map.advanced);

	if (map.advanced != 0U)
	{
		F103_PWM_AdvRegs_t *tim = (F103_PWM_AdvRegs_t *)map.regs;

		tim->CR1 |= (1UL << 7);
		tim->PSC = (uint32_t)psc;
		tim->ARR = (uint32_t)arr;
		tim->EGR = 1U;
		tim->SR = 0U;
		tim->BDTR |= (1UL << 15);
		tim->CR1 |= (1UL << 0);
	}
	else
	{
		F103_PWM_GenRegs_t *tim = (F103_PWM_GenRegs_t *)map.regs;

		tim->CR1 |= (1UL << 7);
		tim->PSC = (uint32_t)psc;
		tim->ARR = (uint32_t)arr;
		tim->EGR = 1U;
		tim->SR = 0U;
		tim->CR1 |= (1UL << 0);
	}
}

/* 设置指定定时器通道的比较值。 */
void F103_PWM_SetCCR(uint8_t timId, uint8_t channel, uint16_t ccr)
{
	F103_PWM_Map_t map;

	if ((channel < 1U) || (channel > 4U))
	{
		return;
	}

	map = F103_PWM_GetMap(timId);
	if (map.regs == 0)
	{
		return;
	}

	F103_PWM_SetChannel(map.regs, map.advanced, channel, ccr);
}

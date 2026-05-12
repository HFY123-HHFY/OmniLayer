#include "f407_adc.h"

typedef struct
{
	volatile uint32_t SR;
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t SMPR1;
	volatile uint32_t SMPR2;
	volatile uint32_t JOFR1;
	volatile uint32_t JOFR2;
	volatile uint32_t JOFR3;
	volatile uint32_t JOFR4;
	volatile uint32_t HTR;
	volatile uint32_t LTR;
	volatile uint32_t SQR1;
	volatile uint32_t SQR2;
	volatile uint32_t SQR3;
	volatile uint32_t JSQR;
	volatile uint32_t JDR1;
	volatile uint32_t JDR2;
	volatile uint32_t JDR3;
	volatile uint32_t JDR4;
	volatile uint32_t DR;
} F407_ADC_Regs_t;

typedef struct
{
	volatile uint32_t CSR;
	volatile uint32_t CCR;
	volatile uint32_t CDR;
} F407_ADC_CommonRegs_t;

typedef struct
{
	volatile uint32_t CR;
	volatile uint32_t PLLCFGR;
	volatile uint32_t CFGR;
	volatile uint32_t CIR;
	volatile uint32_t AHB1RSTR;
	volatile uint32_t AHB2RSTR;
	volatile uint32_t AHB3RSTR;
	volatile uint32_t RESERVED0;
	volatile uint32_t APB1RSTR;
	volatile uint32_t APB2RSTR;
	volatile uint32_t RESERVED1[2];
	volatile uint32_t AHB1ENR;
	volatile uint32_t AHB2ENR;
	volatile uint32_t AHB3ENR;
	volatile uint32_t RESERVED2;
	volatile uint32_t APB1ENR;
	volatile uint32_t APB2ENR;
} F407_ADC_RccRegs_t;

typedef struct
{
	F407_ADC_Regs_t *regs;
	uint32_t rccBit;
} F407_ADC_Map_t;

#define F407_ADC1_BASE      (0x40012000UL)
#define F407_ADC2_BASE      (0x40012100UL)
#define F407_ADC3_BASE      (0x40012200UL)
#define F407_ADC_COMMON_BASE (0x40012300UL)
#define F407_RCC_BASE       (0x40023800UL)

#define F407_RCC_ADC        ((F407_ADC_RccRegs_t *)F407_RCC_BASE)
#define F407_ADC_COMMON     ((F407_ADC_CommonRegs_t *)F407_ADC_COMMON_BASE)

#define F407_ADC_CR2_ADON      (1UL << 0)
#define F407_ADC_CR2_CONT      (1UL << 1)
#define F407_ADC_CR2_EOCS      (1UL << 10)
#define F407_ADC_CR2_ALIGN     (1UL << 11)
#define F407_ADC_CR2_SWSTART   (1UL << 30)
#define F407_ADC_CR2_EXTSEL_MASK (0xFUL << 24)
#define F407_ADC_CR2_EXTEN_MASK  (0x3UL << 28)
#define F407_ADC_SR_EOC        (1UL << 1)
#define F407_ADC_SR_OVR        (1UL << 5)
#define F407_ADC_CR1_RES_MASK  (0x3UL << 24)
#define F407_ADC_CR1_SCAN      (1UL << 8)

/* 把 GPIO 配置为模拟输入（MODER=11，PUPDR=00）。 */
static void F407_ADC_ConfigAnalogPin(void *port, uint16_t pin)
{
	F407_GPIO_Regs_t *gpioPort;
	uint32_t pinIndex;
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

	F407_GPIO_EnablePortClock(port);
	shift = pinIndex * 2U;

	gpioPort->MODER &= ~(0x3UL << shift);
	gpioPort->MODER |= (0x3UL << shift);
	gpioPort->PUPDR &= ~(0x3UL << shift);
}

/* 选择 ADC 实例。adcId: 0->ADC1, 1->ADC2, 2->ADC3。 */
static F407_ADC_Map_t F407_ADC_GetMap(uint8_t adcId)
{
	F407_ADC_Map_t map;

	map.regs = 0;
	map.rccBit = 0U;

	switch (adcId)
	{
	case 0U:
		map.regs = (F407_ADC_Regs_t *)F407_ADC1_BASE;
		map.rccBit = 8U;
		break;
	case 1U:
		map.regs = (F407_ADC_Regs_t *)F407_ADC2_BASE;
		map.rccBit = 9U;
		break;
	case 2U:
		map.regs = (F407_ADC_Regs_t *)F407_ADC3_BASE;
		map.rccBit = 10U;
		break;
	default:
		break;
	}

	return map;
}

/* 设置规则通道采样周期为 144 周期，兼顾速度和稳定性。 */
static void F407_ADC_SetSampleTime(F407_ADC_Regs_t *adc, uint8_t channel)
{
	uint32_t shift;

	if (channel <= 9U)
	{
		shift = (uint32_t)channel * 3U;
		adc->SMPR2 &= ~(0x7UL << shift);
		adc->SMPR2 |= (0x7UL << shift);
	}
	else if (channel <= 18U)
	{
		shift = ((uint32_t)channel - 10U) * 3U;
		adc->SMPR1 &= ~(0x7UL << shift);
		adc->SMPR1 |= (0x7UL << shift);
	}
	else
	{
		/* F407 当前不支持更高通道号。 */
	}
}

void F407_ADC_InitChannel(uint8_t adcId, uint8_t channel, void *port, uint16_t pin)
{
	F407_ADC_Map_t map;
	uint32_t timeout;

	if (channel > 18U)
	{
		return;
	}

	map = F407_ADC_GetMap(adcId);
	if (map.regs == 0)
	{
		return;
	}

	F407_ADC_ConfigAnalogPin(port, pin);

	F407_RCC_ADC->APB2ENR |= (1UL << map.rccBit);

	/* ADC 公共时钟分频：PCLK2/8，保守配置，避免在不同系统时钟下超规格。 */
	F407_ADC_COMMON->CCR &= ~(0x1FUL << 0); /* MULTI=00000，独立模式 */
	F407_ADC_COMMON->CCR &= ~(0x3UL << 16);
	F407_ADC_COMMON->CCR |= (0x3UL << 16);

	map.regs->CR1 = 0U;
	map.regs->CR1 &= ~(F407_ADC_CR1_RES_MASK | F407_ADC_CR1_SCAN);
	map.regs->CR2 &= ~(F407_ADC_CR2_CONT | F407_ADC_CR2_ALIGN | F407_ADC_CR2_EXTSEL_MASK | F407_ADC_CR2_EXTEN_MASK);
	map.regs->CR2 |= F407_ADC_CR2_EOCS;
	map.regs->SQR1 &= ~(0xFUL << 20); /* 规则序列长度 L=0，只转 1 次。 */
	map.regs->SQR3 = (uint32_t)channel;
	F407_ADC_SetSampleTime(map.regs, channel);

	map.regs->CR2 |= F407_ADC_CR2_ADON;
	for (timeout = 0U; timeout < 1000U; ++timeout)
	{
		/* 给 ADC 一点稳定时间。 */
	}
}

uint16_t F407_ADC_ReadChannel(uint8_t adcId, uint8_t channel)
{
	F407_ADC_Map_t map;
	uint32_t timeout;
	uint16_t raw;
	volatile uint32_t dummy;

	if (channel > 18U)
	{
		return 0U;
	}

	map = F407_ADC_GetMap(adcId);
	if (map.regs == 0)
	{
		return 0U;
	}

	map.regs->SQR1 &= ~(0xFUL << 20);
	map.regs->SQR3 = (uint32_t)channel;
	F407_ADC_SetSampleTime(map.regs, channel);
	map.regs->CR1 &= ~(F407_ADC_CR1_RES_MASK | F407_ADC_CR1_SCAN);
	map.regs->CR2 &= ~(F407_ADC_CR2_CONT | F407_ADC_CR2_ALIGN | F407_ADC_CR2_EXTSEL_MASK | F407_ADC_CR2_EXTEN_MASK);
	map.regs->CR2 |= F407_ADC_CR2_EOCS;

	/* 启动新转换前先读一次 DR，尽量排空旧 EOC/旧数据。 */
	dummy = map.regs->DR;
	(void)dummy;
	map.regs->SR = 0U;

	map.regs->CR2 |= F407_ADC_CR2_ADON;
	map.regs->CR2 |= F407_ADC_CR2_SWSTART;

	for (timeout = 0U; timeout < 50000U; ++timeout)
	{
		if ((map.regs->SR & F407_ADC_SR_EOC) != 0U)
		{
			raw = (uint16_t)(map.regs->DR & 0x0FFFU);
			map.regs->SR = 0U;
			return raw;
		}
	}
	return 0U;

}

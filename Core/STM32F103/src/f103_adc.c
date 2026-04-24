#include "f103_adc.h"

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
} F103_ADC_Regs_t;

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
} F103_ADC_RccRegs_t;

typedef struct
{
	F103_ADC_Regs_t *regs;
	uint32_t rccBit;
} F103_ADC_Map_t;

#define F103_ADC1_BASE   (0x40012400UL)
#define F103_ADC2_BASE   (0x40012800UL)
#define F103_RCC_BASE    (0x40021000UL)

#define F103_RCC_ADC     ((F103_ADC_RccRegs_t *)F103_RCC_BASE)

#define F103_ADC_CR2_ADON       (1UL << 0)
#define F103_ADC_CR2_CAL        (1UL << 2)
#define F103_ADC_CR2_RSTCAL     (1UL << 3)
#define F103_ADC_CR2_EXTTRIG    (1UL << 20)
#define F103_ADC_CR2_SWSTART    (1UL << 22)
#define F103_ADC_CR2_EXTSEL_SW  (0x7UL << 17)
#define F103_ADC_SR_EOC         (1UL << 1)

/* 把 GPIO 配置为模拟输入（MODE=00, CNF=00）。 */
static void F103_ADC_ConfigAnalogPin(void *port, uint16_t pin)
{
	F103_GPIO_Regs_t *gpioPort;
	uint32_t pinIndex;
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

	F103_GPIO_EnablePortClock(port);
	shift = (pinIndex & 0x7U) * 4U;

	if (pinIndex < 8U)
	{
		gpioPort->CRL &= ~(0xFUL << shift);
	}
	else
	{
		gpioPort->CRH &= ~(0xFUL << shift);
	}
}

/* 选择 ADC 实例。adcId: 0->ADC1, 1->ADC2。 */
static F103_ADC_Map_t F103_ADC_GetMap(uint8_t adcId)
{
	F103_ADC_Map_t map;

	map.regs = 0;
	map.rccBit = 0U;

	switch (adcId)
	{
	case 0U:
		map.regs = (F103_ADC_Regs_t *)F103_ADC1_BASE;
		map.rccBit = 9U;
		break;
	case 1U:
		map.regs = (F103_ADC_Regs_t *)F103_ADC2_BASE;
		map.rccBit = 10U;
		break;
	default:
		break;
	}

	return map;
}

/* 设置规则通道采样周期为 239.5 周期，提升采样稳定性。 */
static void F103_ADC_SetSampleTime(F103_ADC_Regs_t *adc, uint8_t channel)
{
	uint32_t shift;

	if (channel <= 9U)
	{
		shift = (uint32_t)channel * 3U;
		adc->SMPR2 &= ~(0x7UL << shift);
		adc->SMPR2 |= (0x7UL << shift);
	}
	else if (channel <= 17U)
	{
		shift = ((uint32_t)channel - 10U) * 3U;
		adc->SMPR1 &= ~(0x7UL << shift);
		adc->SMPR1 |= (0x7UL << shift);
	}
	else
	{
		/* F103 当前不支持更高通道号。 */
	}
}

void F103_ADC_InitChannel(uint8_t adcId, uint8_t channel, void *port, uint16_t pin)
{
	F103_ADC_Map_t map;
	uint32_t timeout;

	if (channel > 17U)
	{
		return;
	}

	map = F103_ADC_GetMap(adcId);
	if (map.regs == 0)
	{
		return;
	}

	F103_ADC_ConfigAnalogPin(port, pin);

	/* ADC 时钟预分频：PCLK2/6，满足 F1 ADC 输入时钟要求。 */
	F103_RCC_ADC->CFGR &= ~(0x3UL << 14);
	F103_RCC_ADC->CFGR |= (0x2UL << 14);

	F103_RCC_ADC->APB2ENR |= (1UL << map.rccBit);

	map.regs->CR2 &= ~F103_ADC_CR2_ADON;
	map.regs->CR2 &= ~(0x7UL << 17);
	map.regs->CR2 |= F103_ADC_CR2_EXTSEL_SW;
	map.regs->CR2 &= ~(1UL << 1); /* CONT=0，单次转换。 */
	map.regs->CR1 = 0U;
	map.regs->SQR1 &= ~(0xFUL << 20); /* 规则序列长度 L=0，只转 1 次。 */
	map.regs->SQR3 = (uint32_t)channel;
	F103_ADC_SetSampleTime(map.regs, channel);

	map.regs->CR2 |= F103_ADC_CR2_ADON;
	for (timeout = 0U; timeout < 1000U; ++timeout)
	{
		/* 给 ADC 一点稳定时间。 */
	}
	map.regs->CR2 |= F103_ADC_CR2_RSTCAL;
	while ((map.regs->CR2 & F103_ADC_CR2_RSTCAL) != 0U)
	{
		/* 等待复位校准完成。 */
	}

	map.regs->CR2 |= F103_ADC_CR2_CAL;
	while ((map.regs->CR2 & F103_ADC_CR2_CAL) != 0U)
	{
		/* 等待校准完成。 */
	}
}

uint16_t F103_ADC_ReadChannel(uint8_t adcId, uint8_t channel)
{
	F103_ADC_Map_t map;
	uint32_t timeout;

	if (channel > 17U)
	{
		return 0U;
	}

	map = F103_ADC_GetMap(adcId);
	if (map.regs == 0)
	{
		return 0U;
	}

	map.regs->SQR1 &= ~(0xFUL << 20);
	map.regs->SQR3 = (uint32_t)channel;
	F103_ADC_SetSampleTime(map.regs, channel);

	map.regs->SR &= ~F103_ADC_SR_EOC;
	map.regs->CR2 |= F103_ADC_CR2_EXTTRIG;
	map.regs->CR2 &= ~(0x7UL << 17);
	map.regs->CR2 |= F103_ADC_CR2_EXTSEL_SW;
	map.regs->CR2 |= F103_ADC_CR2_ADON;
	map.regs->CR2 |= F103_ADC_CR2_SWSTART;

	for (timeout = 0U; timeout < 50000U; ++timeout)
	{
		if ((map.regs->SR & F103_ADC_SR_EOC) != 0U)
		{
			return (uint16_t)(map.regs->DR & 0x0FFFU);
		}
	}

	return 0U;
}

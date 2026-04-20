#include "f103_usart.h"

/* USART 外设寄存器映射。 */
typedef struct
{
	volatile uint32_t SR;
	volatile uint32_t DR;
	volatile uint32_t BRR;
	volatile uint32_t CR1;
	volatile uint32_t CR2;
	volatile uint32_t CR3;
	volatile uint32_t GTPR;
} F103_USART_Regs_t;

/* RCC 寄存器映射。 */
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
} F103_RCC_Regs_t;

#define F103_RCC_BASE      (0x40021000UL)
#define F103_USART1_BASE   (0x40013800UL)
#define F103_USART2_BASE   (0x40004400UL)
#define F103_USART3_BASE   (0x40004800UL)
#define F103_NVIC_ISER_BASE (0xE000E100UL)

#define F103_RCC           ((F103_RCC_Regs_t *)F103_RCC_BASE)

#define F103_USART_CR1_UE   (1UL << 13)
#define F103_USART_CR1_RE   (1UL << 2)
#define F103_USART_CR1_TE   (1UL << 3)
#define F103_USART_CR1_RXNEIE (1UL << 5)
#define F103_USART_SR_TXE   (1UL << 7)
#define F103_USART_SR_TC    (1UL << 6)

#define F103_IRQ_USART1     (37U)
#define F103_IRQ_USART2     (38U)
#define F103_IRQ_USART3     (39U)

typedef struct
{
	volatile uint32_t ISER[8];
} F103_NVIC_Regs_t;

typedef struct
{
	F103_USART_Regs_t *regs;
	uint32_t rccBit;
	uint32_t pclkHz;
	uint32_t irqNum;
} F103_USART_Map_t;

static F103_USART_Map_t F103_USART_GetMap(uint8_t usartId)
{
	F103_USART_Map_t map;

	map.regs = 0;
	map.rccBit = 0U;
	map.pclkHz = 0U;
	map.irqNum = 0U;

	switch (usartId)
	{
	case 0U:
		map.regs = (F103_USART_Regs_t *)F103_USART1_BASE;
		map.rccBit = 14U;
		map.pclkHz = 72000000UL;
		map.irqNum = F103_IRQ_USART1;
		break;
	case 1U:
		map.regs = (F103_USART_Regs_t *)F103_USART2_BASE;
		map.rccBit = 17U;
		map.pclkHz = 36000000UL;
		map.irqNum = F103_IRQ_USART2;
		break;
	case 2U:
		map.regs = (F103_USART_Regs_t *)F103_USART3_BASE;
		map.rccBit = 18U;
		map.pclkHz = 36000000UL;
		map.irqNum = F103_IRQ_USART3;
		break;
	default:
		break;
	}

	return map;
}

static uint32_t F103_USART_CalcBrr(uint32_t pclkHz, uint32_t baudRate)
{
	uint32_t usartdiv;
	uint32_t mantissa;
	uint32_t fraction;

	if (baudRate == 0U)
	{
		return 0U;
	}

	usartdiv = pclkHz / baudRate;
	mantissa = usartdiv / 16U;
	fraction = usartdiv % 16U;
	return ((mantissa << 4) | fraction);
}

/* 开启 NVIC 指定中断通道。 */
static void F103_USART_EnableNvicIrq(uint32_t irqNum)
{
	F103_NVIC_Regs_t *nvic;
	uint32_t iserIndex;
	uint32_t iserBit;

	nvic = (F103_NVIC_Regs_t *)F103_NVIC_ISER_BASE;
	iserIndex = irqNum / 32U;
	iserBit = irqNum % 32U;
	nvic->ISER[iserIndex] = (1UL << iserBit);
}

void F103_USART_Init(uint8_t usartId, uint32_t baudRate)
{
	F103_USART_Map_t map;

	map = F103_USART_GetMap(usartId);
	if ((map.regs == 0) || (baudRate == 0U))
	{
		return;
	}

	F103_RCC->APB2ENR |= (1UL << map.rccBit);
	map.regs->CR1 &= ~F103_USART_CR1_UE;
	map.regs->BRR = F103_USART_CalcBrr(map.pclkHz, baudRate);
	map.regs->CR1 = F103_USART_CR1_TE | F103_USART_CR1_RE | F103_USART_CR1_RXNEIE;
	map.regs->CR1 |= F103_USART_CR1_UE;

	F103_USART_EnableNvicIrq(map.irqNum);
}


void F103_USART_WriteByte(uint8_t usartId, uint8_t data)
{
	F103_USART_Map_t map;

	map = F103_USART_GetMap(usartId);
	if (map.regs == 0)
	{
		return;
	}

	while ((map.regs->SR & F103_USART_SR_TXE) == 0U)
	{
		/* 等待发送数据寄存器空。 */
	}

	map.regs->DR = data;
	while ((map.regs->SR & F103_USART_SR_TC) == 0U)
	{
		/* 等待发送完成。 */
	}
}

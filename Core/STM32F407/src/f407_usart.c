#include "f407_usart.h"

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
} F407_USART_Regs_t;

/* RCC 寄存器映射。 */
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
} F407_RCC_Regs_t;

#define F407_RCC_BASE      (0x40023800UL)
#define F407_USART1_BASE   (0x40011000UL)
#define F407_USART2_BASE   (0x40004400UL)
#define F407_USART3_BASE   (0x40004800UL)
#define F407_USART4_BASE   (0x40004C00UL)
#define F407_NVIC_ISER_BASE (0xE000E100UL)

#define F407_RCC           ((F407_RCC_Regs_t *)F407_RCC_BASE)

#define F407_USART_CR1_UE   (1UL << 13)
#define F407_USART_CR1_RE   (1UL << 2)
#define F407_USART_CR1_TE   (1UL << 3)
#define F407_USART_CR1_RXNEIE (1UL << 5)
#define F407_USART_SR_TXE   (1UL << 7)
#define F407_USART_SR_TC    (1UL << 6)

#define F407_IRQ_USART1     (37U)
#define F407_IRQ_USART2     (38U)
#define F407_IRQ_USART3     (39U)
#define F407_IRQ_USART4     (52U)

typedef struct
{
	volatile uint32_t ISER[8];
} F407_NVIC_Regs_t;

typedef struct
{
	F407_USART_Regs_t *regs;
	uint32_t rccBit;
	uint32_t pclkHz;
	uint32_t irqNum;
} F407_USART_Map_t;

static F407_USART_Map_t F407_USART_GetMap(uint8_t usartId)
{
	F407_USART_Map_t map;

	map.regs = 0;
	map.rccBit = 0U;
	map.pclkHz = 0U;
	map.irqNum = 0U;

	switch (usartId)
	{
	case 0U:
		map.regs = (F407_USART_Regs_t *)F407_USART1_BASE;
		map.rccBit = 4U;
		map.pclkHz = 84000000UL;
		map.irqNum = F407_IRQ_USART1;
		break;
	case 1U:
		map.regs = (F407_USART_Regs_t *)F407_USART2_BASE;
		map.rccBit = 17U;
		map.pclkHz = 42000000UL;
		map.irqNum = F407_IRQ_USART2;
		break;
	case 2U:
		map.regs = (F407_USART_Regs_t *)F407_USART3_BASE;
		map.rccBit = 18U;
		map.pclkHz = 42000000UL;
		map.irqNum = F407_IRQ_USART3;
		break;
	case 3U:
		map.regs = (F407_USART_Regs_t *)F407_USART4_BASE;
		map.rccBit = 19U;
		map.pclkHz = 42000000UL;
		map.irqNum = F407_IRQ_USART4;
		break;
	default:
		break;
	}

	return map;
}

static uint32_t F407_USART_CalcBrr(uint32_t pclkHz, uint32_t baudRate)
{
	uint32_t usartdiv;
	uint32_t mantissa;
	uint32_t fraction;

	if (baudRate == 0U)
	{
		return 0U;
	}

	usartdiv = (pclkHz + (baudRate / 2U)) / baudRate;
	mantissa = usartdiv / 16U;
	fraction = usartdiv % 16U;
	return ((mantissa << 4) | fraction);
}

/* 开启 NVIC 指定中断通道。 */
static void F407_USART_EnableNvicIrq(uint32_t irqNum)
{
	F407_NVIC_Regs_t *nvic;
	uint32_t iserIndex;
	uint32_t iserBit;

	nvic = (F407_NVIC_Regs_t *)F407_NVIC_ISER_BASE;
	iserIndex = irqNum / 32U;
	iserBit = irqNum % 32U;
	nvic->ISER[iserIndex] = (1UL << iserBit);
}

/*
 * F407 USART 初始化：
 * 1) 选择对应 USART 实例
 * 2) 打开 APB1/APB2 时钟
 * 3) 配置波特率和收发使能位
 */

void F407_USART_Init(uint8_t usartId, uint32_t baudRate)
{
	F407_USART_Map_t map;

	map = F407_USART_GetMap(usartId);
	if ((map.regs == 0) || (baudRate == 0U))
	{
		return;
	}

	if (usartId == 0U)
	{
		F407_RCC->APB2ENR |= (1UL << map.rccBit);
	}
	else
	{
		F407_RCC->APB1ENR |= (1UL << map.rccBit);
	}
	map.regs->CR1 &= ~F407_USART_CR1_UE;
	map.regs->BRR = F407_USART_CalcBrr(map.pclkHz, baudRate);
	map.regs->CR1 = F407_USART_CR1_TE | F407_USART_CR1_RE | F407_USART_CR1_RXNEIE;
	map.regs->CR1 |= F407_USART_CR1_UE;

	F407_USART_EnableNvicIrq(map.irqNum);
}

/* F407 串口发送 1 字节：等待 TXE 后写 DR，再等待 TC 以保证实际发完。 */
void F407_USART_WriteByte(uint8_t usartId, uint8_t data)
{
	F407_USART_Map_t map;

	map = F407_USART_GetMap(usartId);
	if (map.regs == 0)
	{
		return;
	}

	while ((map.regs->SR & F407_USART_SR_TXE) == 0U)
	{
		/* 等待发送数据寄存器空。 */
	}

	map.regs->DR = data;
	while ((map.regs->SR & F407_USART_SR_TC) == 0U)
	{
		/* 等待发送完成，便于串口助手稳定接收。 */
	}
}

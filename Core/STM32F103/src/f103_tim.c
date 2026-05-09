#include "f103_tim.h"

/* Cortex-M NVIC ISER 寄存器基址。 */
#define F103_NVIC_ISER_BASE  (0xE000E100UL)

/* APB1 总线定时器基址。 */
#define F103_TIM2_BASE       (0x40000000UL)
#define F103_TIM3_BASE       (0x40000400UL)
#define F103_TIM4_BASE       (0x40000800UL)

/* RCC APB1ENR 定时器时钟使能位。 */
#define F103_RCC_APB1ENR_TIM2EN_BIT  (0U)
#define F103_RCC_APB1ENR_TIM3EN_BIT  (1U)
#define F103_RCC_APB1ENR_TIM4EN_BIT  (2U)

/* IRQ 号（STM32F103）。 */
#define F103_IRQ_TIM2  (28U)
#define F103_IRQ_TIM3  (29U)
#define F103_IRQ_TIM4  (30U)

/* 72MHz / 7200 = 10kHz，即 0.1ms 计时节拍。 */
#define F103_TIM_PSC_0P1MS   (7200U - 1U)

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
} F103_TIM_Regs_t;

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

typedef struct
{
	F103_TIM_Regs_t *regs;
	uint32_t rccBit;
	uint32_t irqNum;
} F103_TIM_Map_t;

typedef struct
{
	volatile uint32_t ISER[8];
} F103_NVIC_Regs_t;

#define F103_RCC_BASE  (0x40021000UL)
#define F103_RCC       ((F103_RCC_Regs_t *)F103_RCC_BASE)

static F103_TIM_Map_t F103_TIM_GetMap(uint8_t timId)
{
	F103_TIM_Map_t map;

	map.regs = 0;
	map.rccBit = 0U;
	map.irqNum = 0U;

	switch (timId)
	{
	case 1U: /* API_TIM2 */
		map.regs = (F103_TIM_Regs_t *)F103_TIM2_BASE;
		map.rccBit = F103_RCC_APB1ENR_TIM2EN_BIT;
		map.irqNum = F103_IRQ_TIM2;
		break;
	case 2U: /* API_TIM3 */
		map.regs = (F103_TIM_Regs_t *)F103_TIM3_BASE;
		map.rccBit = F103_RCC_APB1ENR_TIM3EN_BIT;
		map.irqNum = F103_IRQ_TIM3;
		break;
	case 3U: /* API_TIM4 */
		map.regs = (F103_TIM_Regs_t *)F103_TIM4_BASE;
		map.rccBit = F103_RCC_APB1ENR_TIM4EN_BIT;
		map.irqNum = F103_IRQ_TIM4;
		break;
	default:
		break;
	}

	return map;
}

static void F103_TIM_EnableNvicIrq(uint32_t irqNum)
{
	F103_NVIC_Regs_t *nvic;
	uint32_t iserIndex;
	uint32_t iserBit;

	nvic = (F103_NVIC_Regs_t *)F103_NVIC_ISER_BASE;
	iserIndex = irqNum / 32U;
	iserBit = irqNum % 32U;
	nvic->ISER[iserIndex] = (1UL << iserBit);
}

void F103_TIM_PeriodicInit(uint8_t timId, uint32_t periodMs)
{
	F103_TIM_Map_t map;
	uint32_t arr;

	if (periodMs == 0U)
	{
		return;
	}

	map = F103_TIM_GetMap(timId);
	if (map.regs == 0)
	{
		return;
	}

	/* 开启 TIMx 时钟。 */
	F103_RCC->APB1ENR |= (1UL << map.rccBit);

	/* 0.1ms 节拍下，ARR = periodMs * 10 - 1。 */
	arr = (periodMs * 10U) - 1U;

	map.regs->CR1 &= ~(1UL << 0);   /* CEN=0，先停表配置。 */
	map.regs->PSC = F103_TIM_PSC_0P1MS;
	map.regs->ARR = arr;
	map.regs->EGR = 1UL;            /* UG=1，立即更新预分频。 */
	map.regs->SR &= ~(1UL << 0);    /* 清 UIF。 */
	map.regs->DIER |= (1UL << 0);   /* UIE=1，开启更新中断。 */
	map.regs->CR1 |= (1UL << 0);    /* CEN=1，启动计数。 */

	/* 开启 NVIC 对应中断通道。 */
	F103_TIM_EnableNvicIrq(map.irqNum);
}

uint8_t F103_TIM_CheckAndClearUpdateIrq(uint8_t timId)
{
	F103_TIM_Map_t map;

	map = F103_TIM_GetMap(timId);
	if (map.regs == 0)
	{
		return 0U;
	}

	if ((map.regs->SR & 1UL) == 0U)
	{
		return 0U;
	}

	map.regs->SR &= ~1UL;
	return 1U;
}

#include "f407_Encoder.h"

/*
 * F407 编码器底层实现：
 * - TIM1（coreId=0）CH1=PE9(AF1), CH2=PE11(AF1) → 编码器 1
 * - TIM4（coreId=3）CH1=PD12(AF2), CH2=PD13(AF2) → 编码器 2
 * - GPIO 须配置为 AF 模式以接入定时器通道
 * - 定时器编码器模式 3（TI1+TI2 双边沿，4 倍频）
 */

/* TIM 寄存器结构体（通用定时器，也兼容 TIM1 高级定时器的基本寄存器） */
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
} F407_TIM_Regs_t;

/* F407 GPIO 寄存器结构体 */
typedef struct
{
	volatile uint32_t MODER;
	volatile uint32_t OTYPER;
	volatile uint32_t OSPEEDR;
	volatile uint32_t PUPDR;
	volatile uint32_t IDR;
	volatile uint32_t ODR;
	volatile uint32_t BSRR;
	volatile uint32_t LCKR;
	volatile uint32_t AFRL;
	volatile uint32_t AFRH;
} F407_GPIO_Regs_t;

/* RCC 基址 */
#define F407_RCC_BASE       (0x40023800UL)

/* TIM 基址 */
#define F407_TIM1_BASE      (0x40010000UL)
#define F407_TIM4_BASE      (0x40000800UL)

/* GPIO 基址 */
#define F407_GPIOE_BASE     (0x40021000UL)
#define F407_GPIOD_BASE     (0x40020C00UL)

/* 时钟使能位 */
#define F407_APB2ENR_TIM1EN  (0U)   /* APB2 bit 0 */
#define F407_APB1ENR_TIM4EN  (2U)   /* APB1 bit 2 */
#define F407_AHB1ENR_GPIOEEN (4U)   /* AHB1 bit 4 */
#define F407_AHB1ENR_GPIODEN (3U)   /* AHB1 bit 3 */

/* TIM_SMCR 编码器模式 */
#define F407_TIM_SMCR_SMS_Pos    (0U)
#define F407_TIM_SMCR_SMS_Enc3   (3UL)

/* TIM_CCMR1 位 */
#define F407_TIM_CCMR1_CC1S_Pos  (0U)
#define F407_TIM_CCMR1_CC2S_Pos  (8U)
#define F407_TIM_CCMR1_CC1S_TI1  (1UL)
#define F407_TIM_CCMR1_CC2S_TI2  (1UL)
#define F407_TIM_CCMR1_IC1F_Pos  (4U)
#define F407_TIM_CCMR1_IC2F_Pos  (12U)

/* TIM_CCER 位 */
#define F407_TIM_CCER_CC1E       (0U)
#define F407_TIM_CCER_CC2E       (4U)

/* TIM_CR1 位 */
#define F407_TIM_CR1_CEN         (0U)

typedef struct
{
	F407_TIM_Regs_t  *timer;
	void             *gpioPortA;
	uint32_t          pinIndexA;
	uint32_t          afA;         /* 复用功能编号 (AF1=1, AF2=2) */
	void             *gpioPortB;
	uint32_t          pinIndexB;
	uint32_t          afB;
	uint32_t          rccTimBit;
	uint32_t          isApb2;      /* TIM1 在 APB2，TIM4 在 APB1 */
} F407_Encoder_HwMap_t;

static F407_Encoder_HwMap_t F407_Encoder_GetMap(uint8_t coreId)
{
	F407_Encoder_HwMap_t map;

	map.timer     = 0;
	map.gpioPortA = 0;
	map.pinIndexA = 0U;
	map.afA       = 0U;
	map.gpioPortB = 0;
	map.pinIndexB = 0U;
	map.afB       = 0U;
	map.rccTimBit = 0U;
	map.isApb2    = 0U;

	switch (coreId)
	{
	case 0U: /* TIM1: PE9=CH1(AF1), PE11=CH2(AF1) */
		map.timer     = (F407_TIM_Regs_t *)F407_TIM1_BASE;
		map.gpioPortA = (void *)F407_GPIOE_BASE;
		map.pinIndexA = 9U;
		map.afA       = 1U;
		map.gpioPortB = (void *)F407_GPIOE_BASE;
		map.pinIndexB = 11U;
		map.afB       = 1U;
		map.rccTimBit = F407_APB2ENR_TIM1EN;
		map.isApb2    = 1U;
		break;

	case 3U: /* TIM4: PD12=CH1(AF2), PD13=CH2(AF2) */
		map.timer     = (F407_TIM_Regs_t *)F407_TIM4_BASE;
		map.gpioPortA = (void *)F407_GPIOD_BASE;
		map.pinIndexA = 12U;
		map.afA       = 2U;
		map.gpioPortB = (void *)F407_GPIOD_BASE;
		map.pinIndexB = 13U;
		map.afB       = 2U;
		map.rccTimBit = F407_APB1ENR_TIM4EN;
		map.isApb2    = 0U;
		break;

	default:
		break;
	}

	return map;
}

/*
 * 配置 F407 GPIO 为 AF 模式 + 上拉。
 * 直接操作 MODER / PUPDR / AFR 寄存器，不依赖 F407 GPIO API。
 */
static void F407_Encoder_GpioInitAfPullUp(void *portBase, uint32_t pinIndex, uint32_t af)
{
	F407_GPIO_Regs_t *gpio = (F407_GPIO_Regs_t *)portBase;
	uint32_t moderMask;
	uint32_t moderVal;
	uint32_t pupdrMask;
	uint32_t pupdrVal;

	/* MODER: 10 = AF */
	moderMask = 3UL << (pinIndex * 2U);
	moderVal  = 2UL << (pinIndex * 2U);
	gpio->MODER = (gpio->MODER & ~moderMask) | moderVal;

	/* OSPEEDR: 10 = 50MHz */
	gpio->OSPEEDR = (gpio->OSPEEDR & ~(3UL << (pinIndex * 2U))) | (2UL << (pinIndex * 2U));

	/* PUPDR: 01 = 上拉 */
	pupdrMask = 3UL << (pinIndex * 2U);
	pupdrVal  = 1UL << (pinIndex * 2U);
	gpio->PUPDR = (gpio->PUPDR & ~pupdrMask) | pupdrVal;

	/* AFR: 4bit/引脚，低8脚用 AFRL，高8脚用 AFRH */
	if (pinIndex <= 7U)
	{
		uint32_t shift = pinIndex * 4U;
		gpio->AFRL = (gpio->AFRL & ~(0xFUL << shift)) | (af << shift);
	}
	else
	{
		uint32_t shift = (pinIndex - 8U) * 4U;
		gpio->AFRH = (gpio->AFRH & ~(0xFUL << shift)) | (af << shift);
	}
}

void F407_Encoder_Init(uint8_t coreId)
{
	F407_Encoder_HwMap_t map;
	F407_TIM_Regs_t     *tim;
	volatile uint32_t   *ahb1enr;
	volatile uint32_t   *apbXenr;

	map = F407_Encoder_GetMap(coreId);
	tim = map.timer;
	if (tim == 0)
	{
		return;
	}

	/* 1) 使能 GPIO 时钟 */
	ahb1enr = (volatile uint32_t *)(F407_RCC_BASE + 0x30U); /* AHB1ENR */
	{
		uint32_t portBaseA = (uint32_t)(uintptr_t)map.gpioPortA;
		uint32_t portBaseB = (uint32_t)(uintptr_t)map.gpioPortB;

		if (portBaseA == F407_GPIOE_BASE) { *ahb1enr |= (1UL << F407_AHB1ENR_GPIOEEN); }
		if (portBaseB == F407_GPIOE_BASE) { *ahb1enr |= (1UL << F407_AHB1ENR_GPIOEEN); }
		if (portBaseA == F407_GPIOD_BASE) { *ahb1enr |= (1UL << F407_AHB1ENR_GPIODEN); }
		if (portBaseB == F407_GPIOD_BASE) { *ahb1enr |= (1UL << F407_AHB1ENR_GPIODEN); }
	}

	/* 2) 配置两路 GPIO 为 AF 上拉 */
	F407_Encoder_GpioInitAfPullUp(map.gpioPortA, map.pinIndexA, map.afA);
	F407_Encoder_GpioInitAfPullUp(map.gpioPortB, map.pinIndexB, map.afB);

	/* 3) 使能 TIM 时钟 */
	if (map.isApb2 != 0U)
	{
		apbXenr = (volatile uint32_t *)(F407_RCC_BASE + 0x44U); /* APB2ENR */
	}
	else
	{
		apbXenr = (volatile uint32_t *)(F407_RCC_BASE + 0x40U); /* APB1ENR */
	}
	*apbXenr |= (1UL << map.rccTimBit);

	/* 4) 配置定时器编码器模式 */
	tim->CR1 &= ~(1UL << F407_TIM_CR1_CEN);

	/* CCMR1: CC1S=01(TI1→IC1), CC2S=01(TI2→IC2), ICxF=0x3 */
	tim->CCMR1 &= ~((3UL << F407_TIM_CCMR1_CC1S_Pos) | (3UL << F407_TIM_CCMR1_CC2S_Pos));
	tim->CCMR1 |= (F407_TIM_CCMR1_CC1S_TI1 << F407_TIM_CCMR1_CC1S_Pos);
	tim->CCMR1 |= (F407_TIM_CCMR1_CC2S_TI2 << F407_TIM_CCMR1_CC2S_Pos);
	tim->CCMR1 |= (0x3UL << F407_TIM_CCMR1_IC1F_Pos);
	tim->CCMR1 |= (0x3UL << F407_TIM_CCMR1_IC2F_Pos);

	/* CCER: CC1E=1, CC2E=1, 极性默认不反相 */
	tim->CCER |= (1UL << F407_TIM_CCER_CC1E);
	tim->CCER |= (1UL << F407_TIM_CCER_CC2E);

	/* SMCR: SMS=011 编码器模式 3 */
	tim->SMCR &= ~(7UL << F407_TIM_SMCR_SMS_Pos);
	tim->SMCR |= (F407_TIM_SMCR_SMS_Enc3 << F407_TIM_SMCR_SMS_Pos);

	/* ARR=65535, PSC=0 */
	tim->ARR = 0xFFFFU;
	tim->PSC = 0U;
	tim->CNT = 0U;

	/* 更新事件 */
	tim->EGR = 1UL;

	/* 启动 */
	tim->CR1 |= (1UL << F407_TIM_CR1_CEN);
}

int16_t F407_Encoder_GetCount(uint8_t coreId)
{
	F407_Encoder_HwMap_t map;
	F407_TIM_Regs_t     *tim;
	int16_t              val;

	map = F407_Encoder_GetMap(coreId);
	tim = map.timer;
	if (tim == 0)
	{
		return 0;
	}

	val = (int16_t)tim->CNT;
	tim->CNT = 0U;
	return val;
}

#include "f407_Encoder.h"

/*
 * F407 编码器底层实现：
 * - 通过参数接收 portA/pinA、portB/pinB，不硬编码引脚映射；
 * - 自动根据端口基址使能 AHB1 GPIO 时钟；
 * - 自动根据 coreId 使能 APB1/APB2 TIM 时钟；
 * - AF 编号从 TIM 自动推导（TIM1→AF1, TIM2→AF1, TIM3/4/5→AF2, TIM8→AF3）；
 * - GPIO 配置为 AF 模式 + 上拉；
 * - TIM 配置为编码器模式 3。
 */

/* TIM 寄存器结构体（也兼容 TIM1 高级定时器） */
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

/* F407 GPIO 寄存器 */
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

/* 外设基址 */
#define F407_RCC_BASE       (0x40023800UL)
#define F407_TIM1_BASE      (0x40010000UL)
#define F407_TIM2_BASE      (0x40000000UL)
#define F407_TIM3_BASE      (0x40000400UL)
#define F407_TIM4_BASE      (0x40000800UL)
#define F407_TIM5_BASE      (0x40000C00UL)
#define F407_TIM8_BASE      (0x40010400UL)
#define F407_GPIOA_BASE     (0x40020000UL)

/* TIM_SMCR / CCMR1 / CCER / CR1 位定义 */
#define TIM_SMCR_SMS_Pos    (0U)
#define TIM_SMCR_SMS_Enc3   (3UL)
#define TIM_CCMR1_CC1S_Pos  (0U)
#define TIM_CCMR1_CC2S_Pos  (8U)
#define TIM_CCMR1_IC1F_Pos  (4U)
#define TIM_CCMR1_IC2F_Pos  (12U)
#define TIM_CCER_CC1E       (0U)
#define TIM_CCER_CC2E       (4U)
#define TIM_CR1_CEN         (0U)

/* ---- 引脚掩码 → 引脚编号 ---- */
static uint32_t F407_PinToIndex(uint32_t pin)
{
	uint32_t i;
	for (i = 0U; i < 16U; ++i)
	{
		if (pin == (1UL << i)) { return i; }
	}
	return 0xFFFFFFFFUL;
}

/* ---- 根据 coreId 获取 TIM 基址 ---- */
static F407_TIM_Regs_t *F407_Encoder_GetTimer(uint8_t coreId)
{
	switch (coreId)
	{
	case 0U: return (F407_TIM_Regs_t *)F407_TIM1_BASE; /* TIM1 */
	case 1U: return (F407_TIM_Regs_t *)F407_TIM2_BASE; /* TIM2 */
	case 2U: return (F407_TIM_Regs_t *)F407_TIM3_BASE; /* TIM3 */
	case 3U: return (F407_TIM_Regs_t *)F407_TIM4_BASE; /* TIM4 */
	case 4U: return (F407_TIM_Regs_t *)F407_TIM5_BASE; /* TIM5 */
	case 5U: return (F407_TIM_Regs_t *)F407_TIM8_BASE; /* TIM8 */
	default: return 0;
	}
}

/* ---- 根据 TIM 编号推导 AF 编号 ---- */
static uint32_t F407_Encoder_GetAf(uint8_t coreId)
{
	switch (coreId)
	{
	case 0U: return 1U; /* TIM1  → AF1 */
	case 1U: return 1U; /* TIM2  → AF1 */
	case 2U: return 2U; /* TIM3  → AF2 */
	case 3U: return 2U; /* TIM4  → AF2 */
	case 4U: return 2U; /* TIM5  → AF2 */
	case 5U: return 3U; /* TIM8  → AF3 */
	default: return 0U;
	}
}

/* ---- 使能 TIM 时钟（APB1 或 APB2） ---- */
static void F407_Encoder_EnableTimClk(uint8_t coreId)
{
	uint32_t bit;
	volatile uint32_t *enr;

	switch (coreId)
	{
	case 0U: /* TIM1 → APB2 bit 0 */
		enr = (volatile uint32_t *)(F407_RCC_BASE + 0x44U);
		bit = 0U;
		break;
	case 1U: /* TIM2 → APB1 bit 0 */
		enr = (volatile uint32_t *)(F407_RCC_BASE + 0x40U);
		bit = 0U;
		break;
	case 2U: /* TIM3 → APB1 bit 1 */
		enr = (volatile uint32_t *)(F407_RCC_BASE + 0x40U);
		bit = 1U;
		break;
	case 3U: /* TIM4 → APB1 bit 2 */
		enr = (volatile uint32_t *)(F407_RCC_BASE + 0x40U);
		bit = 2U;
		break;
	case 4U: /* TIM5 → APB1 bit 3 */
		enr = (volatile uint32_t *)(F407_RCC_BASE + 0x40U);
		bit = 3U;
		break;
	case 5U: /* TIM8 → APB2 bit 1 */
		enr = (volatile uint32_t *)(F407_RCC_BASE + 0x44U);
		bit = 1U;
		break;
	default:
		return;
	}

	*enr |= (1UL << bit);
}

/* ---- 使能 GPIO 端口时钟（AHB1 bit 0~7 = GPIOA~H） ---- */
static void F407_Encoder_EnableGpioClk(void *port)
{
	volatile uint32_t *ahb1enr = (volatile uint32_t *)(F407_RCC_BASE + 0x30U);
	uint32_t portAddr = (uint32_t)(uintptr_t)port;
	uint32_t bit;

	if (portAddr < F407_GPIOA_BASE) { return; }
	bit = (portAddr - F407_GPIOA_BASE) / 0x400UL;
	if (bit <= 7U)
	{
		*ahb1enr |= (1UL << bit);
	}
}

/* ---- GPIO AF 模式 + 上拉配置 ---- */
static void F407_Encoder_GpioInitAfPu(void *portBase, uint32_t pinIndex, uint32_t af)
{
	F407_GPIO_Regs_t *gpio = (F407_GPIO_Regs_t *)portBase;

	/* MODER: 10 = AF */
	gpio->MODER = (gpio->MODER & ~(3UL << (pinIndex * 2U))) | (2UL << (pinIndex * 2U));

	/* OSPEEDR: 10 = 50MHz */
	gpio->OSPEEDR = (gpio->OSPEEDR & ~(3UL << (pinIndex * 2U))) | (2UL << (pinIndex * 2U));

	/* PUPDR: 01 = 上拉 */
	gpio->PUPDR = (gpio->PUPDR & ~(3UL << (pinIndex * 2U))) | (1UL << (pinIndex * 2U));

	/* AFR: 4bit/引脚，AFRL(0-7), AFRH(8-15) */
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

/* ---- 配置 TIM 为编码器模式 ---- */
static void F407_Encoder_ConfigTimer(F407_TIM_Regs_t *tim)
{
	tim->CR1 &= ~(1UL << TIM_CR1_CEN);

	tim->CCMR1 = (1UL << TIM_CCMR1_CC1S_Pos) |
	             (1UL << TIM_CCMR1_CC2S_Pos) |
	             (3UL << TIM_CCMR1_IC1F_Pos) |
	             (3UL << TIM_CCMR1_IC2F_Pos);

	tim->CCER = (1UL << TIM_CCER_CC1E) | (1UL << TIM_CCER_CC2E);

	tim->SMCR = (TIM_SMCR_SMS_Enc3 << TIM_SMCR_SMS_Pos);

	tim->ARR = 0xFFFFU;
	tim->PSC = 0U;
	tim->CNT = 0U;

	tim->EGR = 1UL;
	tim->CR1 |= (1UL << TIM_CR1_CEN);
}

/* ---- 公共接口 ---- */

void F407_Encoder_Init(uint8_t coreId,
                       void *portA, uint32_t pinA,
                       void *portB, uint32_t pinB)
{
	F407_TIM_Regs_t *tim;
	uint32_t af;
	uint32_t idxA, idxB;

	tim = F407_Encoder_GetTimer(coreId);
	if (tim == 0) { return; }

	af = F407_Encoder_GetAf(coreId);
	if (af == 0U) { return; }

	idxA = F407_PinToIndex(pinA);
	idxB = F407_PinToIndex(pinB);
	if ((idxA > 15U) || (idxB > 15U)) { return; }

	/* 1) 使能 GPIO 时钟 */
	F407_Encoder_EnableGpioClk(portA);
	F407_Encoder_EnableGpioClk(portB);

	/* 2) 配置 GPIO 为 AF 上拉 */
	F407_Encoder_GpioInitAfPu(portA, idxA, af);
	F407_Encoder_GpioInitAfPu(portB, idxB, af);

	/* 3) 使能 TIM 时钟 */
	F407_Encoder_EnableTimClk(coreId);

	/* 4) 配置定时器编码器模式 */
	F407_Encoder_ConfigTimer(tim);
}

int16_t F407_Encoder_GetCount(uint8_t coreId)
{
	F407_TIM_Regs_t *tim;
	int16_t val;

	tim = F407_Encoder_GetTimer(coreId);
	if (tim == 0) { return 0; }

	val = (int16_t)tim->CNT;
	tim->CNT = 0U;
	return val;
}

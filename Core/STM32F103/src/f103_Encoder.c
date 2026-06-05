#include "f103_Encoder.h"

/*
 * F103 编码器底层实现：
 * - 通过参数接收 portA/pinA、portB/pinB，不硬编码引脚映射；
 * - 自动根据端口基址使能 GPIO 时钟，根据 coreId 使能 TIM 时钟；
 * - GPIO 配置为上拉输入模式，TIM 配置为编码器模式 3；
 * - 计数器 CNT 自动跟随编码器方向增减，无需中断。
 */

/* TIM 寄存器结构体（通用定时器） */
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

/* GPIO 寄存器 */
typedef struct
{
	volatile uint32_t CRL;
	volatile uint32_t CRH;
	volatile uint32_t IDR;
	volatile uint32_t ODR;
	volatile uint32_t BSRR;
	volatile uint32_t BRR;
	volatile uint32_t LCKR;
} F103_GPIO_Regs_t;

/* 外设基址 */
#define F103_RCC_BASE      (0x40021000UL)
#define F103_TIM2_BASE     (0x40000000UL)
#define F103_TIM3_BASE     (0x40000400UL)
#define F103_TIM4_BASE     (0x40000800UL)
#define F103_GPIOA_BASE    (0x40010800UL)

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

/* GPIO CRL/CRH 位定义 */
#define GPIO_CR_CNF_Pos(pin)   (((pin) * 4U) + 2U)
#define GPIO_CR_MODE_Pos(pin)  ((pin) * 4U)
#define GPIO_CR_CNF_IN_PU      (2UL)

/* ---- 引脚掩码 → 引脚编号 ---- */
static uint32_t F103_PinToIndex(uint32_t pin)
{
	uint32_t i;
	for (i = 0U; i < 16U; ++i)
	{
		if (pin == (1UL << i)) { return i; }
	}
	return 0xFFFFFFFFUL;
}

/* ---- 根据 coreId 获取 TIM 基址 ---- */
static F103_TIM_Regs_t *F103_Encoder_GetTimer(uint8_t coreId)
{
	switch (coreId)
	{
	case 1U: return (F103_TIM_Regs_t *)F103_TIM2_BASE; /* TIM2 */
	case 2U: return (F103_TIM_Regs_t *)F103_TIM3_BASE; /* TIM3 */
	case 3U: return (F103_TIM_Regs_t *)F103_TIM4_BASE; /* TIM4 */
	default: return 0;
	}
}

/* ---- 根据 coreId 获取 APB1ENR bit ---- */
static uint32_t F103_Encoder_GetApb1Bit(uint8_t coreId)
{
	switch (coreId)
	{
	case 1U: return 0U; /* TIM2 */
	case 2U: return 1U; /* TIM3 */
	case 3U: return 2U; /* TIM4 */
	default: return 0xFFFFFFFFUL;
	}
}

/* ---- 使能 GPIO 端口时钟（APB2 bit 2~6 = GPIOA~E） ---- */
static void F103_Encoder_EnableGpioClk(void *port)
{
	volatile uint32_t *apb2enr = (volatile uint32_t *)(F103_RCC_BASE + 0x18U);
	uint32_t portAddr = (uint32_t)(uintptr_t)port;
	uint32_t bit;

	if (portAddr < F103_GPIOA_BASE) { return; }
	bit = ((portAddr - F103_GPIOA_BASE) / 0x400UL) + 2U;
	if (bit <= 6U)
	{
		*apb2enr |= (1UL << bit);
	}
}

/* ---- GPIO 上拉输入配置 ---- */
static void F103_Encoder_GpioInitPu(void *portBase, uint32_t pinIndex)
{
	F103_GPIO_Regs_t *gpio = (F103_GPIO_Regs_t *)portBase;
	uint32_t odr;

	odr  = gpio->ODR | (1UL << pinIndex);
	gpio->ODR = odr;

	if (pinIndex <= 7U)
	{
		gpio->CRL &= ~((3UL << GPIO_CR_CNF_Pos(pinIndex)) |
		               (3UL << GPIO_CR_MODE_Pos(pinIndex)));
		gpio->CRL |= (GPIO_CR_CNF_IN_PU << GPIO_CR_CNF_Pos(pinIndex));
	}
	else
	{
		uint32_t pi = pinIndex - 8U;
		gpio->CRH &= ~((3UL << GPIO_CR_CNF_Pos(pi)) |
		               (3UL << GPIO_CR_MODE_Pos(pi)));
		gpio->CRH |= (GPIO_CR_CNF_IN_PU << GPIO_CR_CNF_Pos(pi));
	}
}

/* ---- 配置 TIM 为编码器模式 ---- */
static void F103_Encoder_ConfigTimer(F103_TIM_Regs_t *tim, uint32_t apb1Bit)
{
	volatile uint32_t *apb1enr = (volatile uint32_t *)(F103_RCC_BASE + 0x1CU);
	volatile uint32_t *apb2enr = (volatile uint32_t *)(F103_RCC_BASE + 0x18U);

	/* 使能 AFIO 时钟 */
	*apb2enr |= (1UL << 0U);

	/* 使能 TIM 时钟 */
	*apb1enr |= (1UL << apb1Bit);

	/* 停表配置 */
	tim->CR1 &= ~(1UL << TIM_CR1_CEN);

	/* CCMR1: CC1S=01(TI1), CC2S=01(TI2), ICxF=0x3 */
	tim->CCMR1 = (1UL << TIM_CCMR1_CC1S_Pos) |
	             (1UL << TIM_CCMR1_CC2S_Pos) |
	             (3UL << TIM_CCMR1_IC1F_Pos) |
	             (3UL << TIM_CCMR1_IC2F_Pos);

	/* CCER: CC1E=1, CC2E=1 */
	tim->CCER = (1UL << TIM_CCER_CC1E) | (1UL << TIM_CCER_CC2E);

	/* SMCR: SMS=011 编码器模式 3 */
	tim->SMCR = (TIM_SMCR_SMS_Enc3 << TIM_SMCR_SMS_Pos);

	/* ARR=65535, PSC=0 */
	tim->ARR = 0xFFFFU;
	tim->PSC = 0U;

	/* CNT 清 0，更新事件加载影子寄存器 */
	tim->CNT = 0U;
	tim->EGR = 1UL;

	/* 启动 */
	tim->CR1 |= (1UL << TIM_CR1_CEN);
}

/* ---- 公共接口 ---- */

void F103_Encoder_Init(uint8_t coreId,
                       void *portA, uint32_t pinA,
                       void *portB, uint32_t pinB)
{
	F103_TIM_Regs_t *tim;
	uint32_t apb1Bit;
	uint32_t idxA, idxB;

	tim = F103_Encoder_GetTimer(coreId);
	if (tim == 0) { return; }

	apb1Bit = F103_Encoder_GetApb1Bit(coreId);
	if (apb1Bit == 0xFFFFFFFFUL) { return; }

	idxA = F103_PinToIndex(pinA);
	idxB = F103_PinToIndex(pinB);
	if ((idxA > 15U) || (idxB > 15U)) { return; }

	/* 1) 使能 GPIO 时钟 */
	F103_Encoder_EnableGpioClk(portA);
	F103_Encoder_EnableGpioClk(portB);

	/* 2) 配置 GPIO 为上拉输入 */
	F103_Encoder_GpioInitPu(portA, idxA);
	F103_Encoder_GpioInitPu(portB, idxB);

	/* 3) 配置 TIM 编码器模式 */
	F103_Encoder_ConfigTimer(tim, apb1Bit);
}

int16_t F103_Encoder_GetCount(uint8_t coreId)
{
	F103_TIM_Regs_t *tim;
	int16_t val;

	tim = F103_Encoder_GetTimer(coreId);
	if (tim == 0) { return 0; }

	val = (int16_t)tim->CNT;
	tim->CNT = 0U;
	return val;
}

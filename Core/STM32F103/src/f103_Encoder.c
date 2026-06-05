#include "f103_Encoder.h"

/*
 * F103 编码器底层实现：
 * - TIM2（coreId=1）CH1=PA0, CH2=PA1 → 编码器 1
 * - TIM4（coreId=3）CH1=PB6, CH2=PB7 → 编码器 2
 * - 使用定时器编码器模式 3（TI1+TI2 双边沿计数，4 倍频）
 * - 计数器 CNT 自动跟随编码器方向增减，无需中断
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

/* GPIO 寄存器结构体 */
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

/* RCC 基址与寄存器 */
#define F103_RCC_BASE    (0x40021000UL)
#define F103_APB2ENR_OFF (0x18U)

/* TIM 基址 */
#define F103_TIM2_BASE   (0x40000000UL)
#define F103_TIM4_BASE   (0x40000800UL)

/* TIM 可选重映射基址 */
#define F103_AFIO_BASE   (0x40010000UL)

/* GPIO 基址 */
#define F103_GPIOA_BASE  (0x40010800UL)
#define F103_GPIOB_BASE  (0x40010C00UL)

/* APB1ENR 定时器时钟使能位 */
#define F103_RCC_APB1ENR_TIM2EN  (0U)
#define F103_RCC_APB1ENR_TIM4EN  (2U)

/* TIM_SMCR 位定义 */
#define F103_TIM_SMCR_SMS_Pos    (0U)
#define F103_TIM_SMCR_SMS_Enc3   (3UL)   /* 编码器模式 3 */

/* TIM_CCMR1 位定义 */
#define F103_TIM_CCMR1_CC1S_Pos  (0U)
#define F103_TIM_CCMR1_CC2S_Pos  (8U)
#define F103_TIM_CCMR1_CC1S_TI1  (1UL)   /* CC1 通道配置为输入，IC1 映射到 TI1 */
#define F103_TIM_CCMR1_CC2S_TI2  (1UL)   /* CC2 通道配置为输入，IC2 映射到 TI2 */
#define F103_TIM_CCMR1_IC1F_Pos  (4U)
#define F103_TIM_CCMR1_IC2F_Pos  (12U)

/* TIM_CCER 位定义 */
#define F103_TIM_CCER_CC1E       (0U)
#define F103_TIM_CCER_CC2E       (4U)

/* TIM_CR1 位定义 */
#define F103_TIM_CR1_CEN         (0U)

/* GPIO CRL 位定义 */
#define F103_GPIO_CRL_CNF_Pos(pin)   (((pin) * 4U) + 2U)
#define F103_GPIO_CRL_MODE_Pos(pin)  ((pin) * 4U)
#define F103_GPIO_CRL_CNF_IN_PU  (2UL) /* 10: 上拉/下拉输入 */

/* GPIO CRH 位定义（pin 8~15） */
#define F103_GPIO_CRH_CNF_Pos(pin)   ((((pin) - 8U) * 4U) + 2U)
#define F103_GPIO_CRH_MODE_Pos(pin)  (((pin) - 8U) * 4U)
#define F103_GPIO_CRH_CNF_IN_PU  (2UL)

typedef struct
{
	F103_TIM_Regs_t  *timer;
	void             *gpioPortA;
	uint32_t          pinA;
	uint32_t          pinIndexA;
	void             *gpioPortB;
	uint32_t          pinB;
	uint32_t          pinIndexB;
	uint32_t          rccTimBit;
} F103_Encoder_HwMap_t;

/*
 * 根据 coreId 获取该编码器的硬件映射。
 * coreId 1 = TIM2, coreId 3 = TIM4。
 */
static F103_Encoder_HwMap_t F103_Encoder_GetMap(uint8_t coreId)
{
	F103_Encoder_HwMap_t map;

	/* 初始化为空 */
	map.timer     = 0;
	map.gpioPortA = 0;
	map.pinA      = 0U;
	map.pinIndexA = 0U;
	map.gpioPortB = 0;
	map.pinB      = 0U;
	map.pinIndexB = 0U;
	map.rccTimBit = 0U;

	switch (coreId)
	{
	case 1U: /* TIM2: PA0=CH1, PA1=CH2 */
		map.timer     = (F103_TIM_Regs_t *)F103_TIM2_BASE;
		map.gpioPortA = (void *)F103_GPIOA_BASE;
		map.pinA      = (1UL << 0U);
		map.pinIndexA = 0U;
		map.gpioPortB = (void *)F103_GPIOA_BASE;
		map.pinB      = (1UL << 1U);
		map.pinIndexB = 1U;
		map.rccTimBit = F103_RCC_APB1ENR_TIM2EN;
		break;

	case 3U: /* TIM4: PB6=CH1, PB7=CH2 */
		map.timer     = (F103_TIM_Regs_t *)F103_TIM4_BASE;
		map.gpioPortA = (void *)F103_GPIOB_BASE;
		map.pinA      = (1UL << 6U);
		map.pinIndexA = 6U;
		map.gpioPortB = (void *)F103_GPIOB_BASE;
		map.pinB      = (1UL << 7U);
		map.pinIndexB = 7U;
		map.rccTimBit = F103_RCC_APB1ENR_TIM4EN;
		break;

	default:
		break;
	}

	return map;
}

/*
 * 为单个引脚配置上拉输入模式。
 * 替代直接调用 API_GPIO，避免在 Core 层依赖 API 层。
 */
static void F103_Encoder_GpioInitInputPullUp(void *portBase, uint32_t pinIndex)
{
	F103_GPIO_Regs_t *gpio;
	uint32_t odrVal;

	gpio = (F103_GPIO_Regs_t *)portBase;

	/* 先写 ODR 使能上拉（ODR=1 + CNF=10 = 上拉输入） */
	odrVal = gpio->ODR;
	odrVal |= (1UL << pinIndex);
	gpio->ODR = odrVal;

	if (pinIndex <= 7U)
	{
		/* CRL 控制 Pin0~Pin7 */
		gpio->CRL &= ~(3UL << F103_GPIO_CRL_CNF_Pos(pinIndex));
		gpio->CRL &= ~(3UL << F103_GPIO_CRL_MODE_Pos(pinIndex));
		gpio->CRL |= (F103_GPIO_CRL_CNF_IN_PU << F103_GPIO_CRL_CNF_Pos(pinIndex));
	}
	else
	{
		/* CRH 控制 Pin8~Pin15 */
		gpio->CRH &= ~(3UL << F103_GPIO_CRH_CNF_Pos(pinIndex));
		gpio->CRH &= ~(3UL << F103_GPIO_CRH_MODE_Pos(pinIndex));
		gpio->CRH |= (F103_GPIO_CRH_CNF_IN_PU << F103_GPIO_CRH_CNF_Pos(pinIndex));
	}
}

/* 读 F103 RCC APB1ENR */
static volatile uint32_t *F103_Encoder_GetRccApb1Enr(void)
{
	return (volatile uint32_t *)(F103_RCC_BASE + 0x1CU); /* APB1ENR 偏移 0x1C */
}

void F103_Encoder_Init(uint8_t coreId)
{
	F103_Encoder_HwMap_t map;
	F103_TIM_Regs_t     *tim;
	volatile uint32_t   *apb1enr;

	map = F103_Encoder_GetMap(coreId);
	tim = map.timer;
	if (tim == 0)
	{
		return;
	}

	/* 1) 使能 GPIO 时钟（端口 A 和 B 都在 APB2 上）*/
	{
		volatile uint32_t *apb2enr = (volatile uint32_t *)(F103_RCC_BASE + 0x18U);
		uint32_t portBaseA = (uint32_t)(uintptr_t)map.gpioPortA;
		uint32_t portBaseB = (uint32_t)(uintptr_t)map.gpioPortB;

		if (portBaseA == F103_GPIOA_BASE)  { *apb2enr |= (1UL << 2U); }
		if (portBaseA == F103_GPIOB_BASE)  { *apb2enr |= (1UL << 3U); }
		if (portBaseB == F103_GPIOB_BASE)  { *apb2enr |= (1UL << 3U); }
		(void)portBaseB;
	}

	/* 2) 使能 AFIO 时钟（可能需要重映射，先开着） */
	{
		volatile uint32_t *apb2enr = (volatile uint32_t *)(F103_RCC_BASE + 0x18U);
		*apb2enr |= (1UL << 0U); /* AFIOEN */
	}

	/* 3) 配置两路 GPIO 为上拉输入 */
	F103_Encoder_GpioInitInputPullUp(map.gpioPortA, map.pinIndexA);
	F103_Encoder_GpioInitInputPullUp(map.gpioPortB, map.pinIndexB);

	/* 4) 使能 TIM 时钟 */
	apb1enr = F103_Encoder_GetRccApb1Enr();
	*apb1enr |= (1UL << map.rccTimBit);

	/* 5) 配置定时器编码器模式 */
	/* 停表 */
	tim->CR1 &= ~(1UL << F103_TIM_CR1_CEN);

	/* CCMR1: CC1S=01(TI1→IC1), CC2S=01(TI2→IC2), 滤波器=0x3 */
	tim->CCMR1 &= ~((3UL << F103_TIM_CCMR1_CC1S_Pos) | (3UL << F103_TIM_CCMR1_CC2S_Pos));
	tim->CCMR1 |= (F103_TIM_CCMR1_CC1S_TI1  << F103_TIM_CCMR1_CC1S_Pos);
	tim->CCMR1 |= (F103_TIM_CCMR1_CC2S_TI2  << F103_TIM_CCMR1_CC2S_Pos);
	/* 输入滤波器 fSAMPLING=fDTS/8, N=8 → ICxF=0x3 */
	tim->CCMR1 |= (0x3UL << F103_TIM_CCMR1_IC1F_Pos);
	tim->CCMR1 |= (0x3UL << F103_TIM_CCMR1_IC2F_Pos);

	/* CCER: CC1E=1, CC2E=1, 极性不反相 */
	tim->CCER |= (1UL << F103_TIM_CCER_CC1E);
	tim->CCER |= (1UL << F103_TIM_CCER_CC2E);

	/* SMCR: SMS=011 编码器模式 3 */
	tim->SMCR &= ~(7UL << F103_TIM_SMCR_SMS_Pos);
	tim->SMCR |= (F103_TIM_SMCR_SMS_Enc3 << F103_TIM_SMCR_SMS_Pos);

	/* ARR = 65535（最大计数范围） */
	tim->ARR = 0xFFFFU;
	tim->PSC = 0U;

	/* CNT 清 0 */
	tim->CNT = 0U;

	/* 产生更新事件，加载影子寄存器 */
	tim->EGR = 1UL;

	/* 启动定时器 */
	tim->CR1 |= (1UL << F103_TIM_CR1_CEN);
}

int16_t F103_Encoder_GetCount(uint8_t coreId)
{
	F103_Encoder_HwMap_t map;
	F103_TIM_Regs_t     *tim;
	int16_t              val;

	map = F103_Encoder_GetMap(coreId);
	tim = map.timer;
	if (tim == 0)
	{
		return 0;
	}

	val = (int16_t)tim->CNT;
	tim->CNT = 0U;
	return val;
}

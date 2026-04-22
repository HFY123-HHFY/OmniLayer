#include "sys.h"

/*
    * F103 “硬件配置实现层”
    * 配置 GPIO 输入上拉、AFIO EXTI 映射、EXTI 触发沿、NVIC 优先级和使能
*/

/* F103 EXTI 寄存器映射（最小字段集）。 */
typedef struct
{
	volatile uint32_t IMR;
	volatile uint32_t EMR;
	volatile uint32_t RTSR;
	volatile uint32_t FTSR;
	volatile uint32_t SWIER;
	volatile uint32_t PR;
} F103_EXTI_Regs_t;

/* F103 AFIO 寄存器映射：用于 EXTI 端口重映射。 */
typedef struct
{
	volatile uint32_t EVCR;
	volatile uint32_t MAPR;
	volatile uint32_t EXTICR[4];
	volatile uint32_t MAPR2;
} F103_AFIO_Regs_t;

/* F103 NVIC 寄存器映射：用于配置中断优先级和使能。 */
typedef struct
{
	volatile uint32_t ISER[8];
	uint32_t RESERVED0[24];
	volatile uint32_t ICER[8];
	uint32_t RESERVED1[24];
	volatile uint32_t ISPR[8];
	uint32_t RESERVED2[24];
	volatile uint32_t ICPR[8];
	uint32_t RESERVED3[24];
	volatile uint32_t IABR[8];
	uint32_t RESERVED4[56];
	volatile uint8_t IP[240];
} F103_NVIC_Regs_t;

#define F103_AFIO_BASE  (0x40010000UL)
#define F103_EXTI_BASE  (0x40010400UL)
#define F103_NVIC_BASE  (0xE000E100UL)

#define AFIO   ((F103_AFIO_Regs_t *)F103_AFIO_BASE)
#define EXTI   ((F103_EXTI_Regs_t *)F103_EXTI_BASE)
#define NVIC   ((F103_NVIC_Regs_t *)F103_NVIC_BASE)

/*
 * 把 GPIO 端口地址转换为 AFIO EXTICR 端口编码：
 * GPIOA->0, GPIOB->1, GPIOC->2, GPIOD->3, GPIOE->4。
 */
static uint8_t F103_SYS_GetPortCode(void *port)
{
	if (port == GPIOA)
	{
		return 0U;
	}
	if (port == GPIOB)
	{
		return 1U;
	}
	if (port == GPIOC)
	{
		return 2U;
	}
	if (port == GPIOD)
	{
		return 3U;
	}
	if (port == GPIOE)
	{
		return 4U;
	}

	return 0xFFU;
}

/*
 * F103 EXTI 初始化：
 * 1) 把指定引脚配置为输入上拉；
 * 2) 配置 AFIO EXTICR 的端口映射；
 * 3) 配置 EXTI 触发沿与中断屏蔽；
 * 4) 配置 NVIC 优先级并使能 IRQ。
 */
void F103_SYS_EXTI_Init(void *port, uint8_t lineIndex, SYS_EXTI_Trigger_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority)
{
	uint32_t extiMask;
	uint32_t exticrIndex;
	uint32_t exticrShift;
	uint8_t portCode;

	if ((port == 0) || (lineIndex > 15U))
	{
		return;
	}

	API_GPIO_InitInputPullUp(port, (uint16_t)(1UL << lineIndex));

	F103_RCC->APB2ENR |= (1UL << 0); /* 使能 AFIO 时钟。 */
	portCode = F103_SYS_GetPortCode(port);
	if (portCode > 4U)
	{
		return;
	}

	extiMask = (uint32_t)1UL << lineIndex;
	exticrIndex = lineIndex >> 2U;
	exticrShift = (lineIndex & 0x3U) * 4U;

	AFIO->EXTICR[exticrIndex] &= ~(0xFUL << exticrShift);
	AFIO->EXTICR[exticrIndex] |= ((uint32_t)portCode << exticrShift);

	EXTI->IMR |= extiMask;
	EXTI->EMR &= ~extiMask;

	if ((trigger & SYS_EXTI_TRIGGER_RISING) != 0U)
	{
		EXTI->RTSR |= extiMask;
	}
	else
	{
		EXTI->RTSR &= ~extiMask;
	}

	if ((trigger & SYS_EXTI_TRIGGER_FALLING) != 0U)
	{
		EXTI->FTSR |= extiMask;
	}
	else
	{
		EXTI->FTSR &= ~extiMask;
	}

	EXTI->PR = extiMask;
	/* Cortex-M 中优先级高 4bit 有效，低位写 0 无影响。 */
	NVIC->IP[irqn] = (uint8_t)(((uint32_t)preemptPriority << 4U) | (subPriority & 0x0FU));
	NVIC->ISER[irqn >> 5U] = (uint32_t)(1UL << (irqn & 0x1FU));
}

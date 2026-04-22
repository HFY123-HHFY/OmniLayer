#include "sys.h"

/* EXTI 线到 NVIC 中断通道映射。 */
#define SYS_EXTI0_IRQn      (6U)
#define SYS_EXTI1_IRQn      (7U)
#define SYS_EXTI2_IRQn      (8U)
#define SYS_EXTI3_IRQn      (9U)
#define SYS_EXTI4_IRQn      (10U)
#define SYS_EXTI9_5_IRQn    (23U)
#define SYS_EXTI15_10_IRQn  (40U)
#define SYS_INVALID_IRQN    (0xFFFFFFFFUL)

/* 当前已注册 EXTI 线号。 */
static uint8_t s_sysExtiLineIndex = 0xFFU;

/* F103/F407 仅 EXTI 基地址不同，按目标 MCU 选择。 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#define SYS_EXTI_BASE (0x40010400UL)
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#define SYS_EXTI_BASE (0x40013C00UL)
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

typedef struct
{
	volatile uint32_t IMR;
	volatile uint32_t EMR;
	volatile uint32_t RTSR;
	volatile uint32_t FTSR;
	volatile uint32_t SWIER;
	volatile uint32_t PR;
} SYS_EXTI_Regs_t;

#define SYS_EXTI ((SYS_EXTI_Regs_t *)SYS_EXTI_BASE)

static uint8_t SYS_GetPinIndex(uint16_t pin)
{
	uint8_t index;

	for (index = 0U; index < 16U; ++index)
	{
		if (pin == (uint16_t)(1U << index))
		{
			return index;
		}
	}

	return 0xFFU;
}

static uint32_t SYS_GetExtiIrq(uint8_t lineIndex)
{
	if (lineIndex == 0U)
	{
		return (uint32_t)SYS_EXTI0_IRQn;
	}
	if (lineIndex == 1U)
	{
		return (uint32_t)SYS_EXTI1_IRQn;
	}
	if (lineIndex == 2U)
	{
		return (uint32_t)SYS_EXTI2_IRQn;
	}
	if (lineIndex == 3U)
	{
		return (uint32_t)SYS_EXTI3_IRQn;
	}
	if (lineIndex == 4U)
	{
		return (uint32_t)SYS_EXTI4_IRQn;
	}
	if ((lineIndex >= 5U) && (lineIndex <= 9U))
	{
		return (uint32_t)SYS_EXTI9_5_IRQn;
	}
	if ((lineIndex >= 10U) && (lineIndex <= 15U))
	{
		return (uint32_t)SYS_EXTI15_10_IRQn;
	}

	return SYS_INVALID_IRQN;
}

/*
 * 系统层入口：
 * 当前仅保留系统初始化占位，避免系统层与具体 BSP 外设中断耦合。
 */
void SYS_Init(void)
{
	/* 需要全局系统初始化时在此扩展。 */
}

void SYS_EXTI_Register(void *port, uint16_t pin, SYS_EXTI_Trigger_t trigger,
	uint8_t preemptPriority, uint8_t subPriority)
{
	uint8_t lineIndex;
	uint32_t irqn;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	lineIndex = SYS_GetPinIndex(pin);
	if (lineIndex > 15U)
	{
		return;
	}

	irqn = SYS_GetExtiIrq(lineIndex);
	if (irqn == SYS_INVALID_IRQN)
	{
		return;
	}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	F103_SYS_EXTI_Init(port, lineIndex, trigger, irqn, preemptPriority, subPriority);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	F407_SYS_EXTI_Init(port, lineIndex, trigger, irqn, preemptPriority, subPriority);
#endif

	s_sysExtiLineIndex = lineIndex;
}

uint8_t SYS_EXTI_IRQHandlerGroup(uint8_t startLine, uint8_t endLine)
{
	uint32_t lineMask;

	if ((s_sysExtiLineIndex < startLine) ||
		(s_sysExtiLineIndex > endLine) ||
		(s_sysExtiLineIndex > 15U))
	{
		return 0U;
	}

	lineMask = (uint32_t)1UL << s_sysExtiLineIndex;
	if ((SYS_EXTI->PR & lineMask) == 0U)
	{
		return 0U;
	}

	SYS_EXTI->PR = lineMask;
	return 1U;
}

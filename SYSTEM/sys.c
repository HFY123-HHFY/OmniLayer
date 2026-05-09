#include "sys.h"
#include "exti.h"

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

static uint8_t SYS_GetPinIndex(uint32_t pin)
{
	uint8_t index;

	for (index = 0U; index < 16U; ++index)
	{
		if (pin == (uint32_t)(1UL << index))
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

void SYS_EXTI_Register(void *port, uint32_t pin, SYS_EXTI_Trigger_t trigger,
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

	API_EXTI_Init(port, pin, (API_EXTI_Trigger_t)trigger, irqn, preemptPriority, subPriority);

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
	if (API_EXTI_IsPendingAndClear(s_sysExtiLineIndex) == 0U)
	{
		return 0U;
	}
	(void)lineMask;
	return 1U;
}

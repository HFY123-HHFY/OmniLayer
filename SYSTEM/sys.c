#include "sys.h"

#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#include "G3507_sys.h"
#endif

/* EXTI 线到 NVIC 中断通道映射。 */
#define SYS_EXTI0_IRQn      (6U)
#define SYS_EXTI1_IRQn      (7U)
#define SYS_EXTI2_IRQn      (8U)
#define SYS_EXTI3_IRQn      (9U)
#define SYS_EXTI4_IRQn      (10U)
#define SYS_EXTI9_5_IRQn    (23U)
#define SYS_EXTI15_10_IRQn  (40U)

uint8_t SYS_EXTI_GetLineIndex(uint32_t pin)
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

void SYS_Init(void)
{
	#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	G3507_SYS_Init();
	#else
	/* 非 G3507 平台保持现状。 */
	#endif
}

uint32_t SYS_EXTI_GetIrqn(void *port, uint32_t pin)
{
	if ((port == 0) || (pin == 0U))
	{
		return SYS_EXTI_INVALID_IRQN;
	}

	#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	/* G3507 GPIOA/GPIOB 使用端口中断，支持 B24-B27 等高位引脚。
	 * 仅需根据端口选择 IRQn，不再限制 lineIndex <= 15。
	 */
	if (port == GPIOA)
	{
		return (uint32_t)GPIOA_INT_IRQn;
	}
	if (port == GPIOB)
	{
		return (uint32_t)GPIOB_INT_IRQn;
	}
	return SYS_EXTI_INVALID_IRQN;
	#else
	uint8_t lineIndex;

	lineIndex = SYS_EXTI_GetLineIndex(pin);
	if (lineIndex > 15U)
	{
		return SYS_EXTI_INVALID_IRQN;
	}
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
	return SYS_EXTI_INVALID_IRQN;
	#endif
}

uint8_t SYS_EXTI_LineInGroup(uint32_t pin, uint8_t startLine, uint8_t endLine)
{
	uint8_t lineIndex;

	lineIndex = SYS_EXTI_GetLineIndex(pin);
	if ((lineIndex > 15U) || (lineIndex < startLine) || (lineIndex > endLine))
	{
		return 0U;
	}

	return 1U;
}

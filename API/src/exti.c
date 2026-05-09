#include "exti.h"

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_exti.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_exti.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
/* G3507 EXTI 后续补充。 */
#else
#error "Unsupported ENROLL_MCU_TARGET for API EXTI backend."
#endif

void API_EXTI_Init(void *port, uint32_t pin, API_EXTI_Trigger_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority)
{
	uint8_t lineIndex;
	uint8_t i;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	lineIndex = 0xFFU;
	for (i = 0U; i < 16U; ++i)
	{
		if (pin == (uint32_t)(1UL << i))
		{
			lineIndex = i;
			break;
		}
	}

	if (lineIndex > 15U)
	{
		return;
	}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	F103_EXTI_Init(port, lineIndex, trigger, irqn, preemptPriority, subPriority);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	F407_EXTI_Init(port, lineIndex, trigger, irqn, preemptPriority, subPriority);
#else
	(void)trigger;
	(void)irqn;
	(void)preemptPriority;
	(void)subPriority;
#endif
}

uint8_t API_EXTI_IsPendingAndClear(uint8_t lineIndex)
{
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	return F103_EXTI_IsPendingAndClear(lineIndex);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	return F407_EXTI_IsPendingAndClear(lineIndex);
#else
	(void)lineIndex;
	return 0U;
#endif
}

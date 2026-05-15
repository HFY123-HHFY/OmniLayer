#include "G3507_exti.h"

#include "exti.h"
#include "gpio.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

#define G3507_EXTI_POLARITY_CASE(index) \
	case DL_GPIO_PIN_##index: \
		if ((trigger & API_EXTI_TRIGGER_RISING) != 0U) \
		{ \
			if ((trigger & API_EXTI_TRIGGER_FALLING) != 0U) \
			{ \
				return DL_GPIO_PIN_##index##_EDGE_RISE_FALL; \
			} \
			return DL_GPIO_PIN_##index##_EDGE_RISE; \
		} \
		if ((trigger & API_EXTI_TRIGGER_FALLING) != 0U) \
		{ \
			return DL_GPIO_PIN_##index##_EDGE_FALL; \
		} \
		return DL_GPIO_PIN_##index##_EDGE_DISABLE

static uint32_t G3507_EXTI_GetPolarity(uint32_t pin, uint32_t trigger)
{
	switch (pin)
	{
		G3507_EXTI_POLARITY_CASE(0);
		G3507_EXTI_POLARITY_CASE(1);
		G3507_EXTI_POLARITY_CASE(2);
		G3507_EXTI_POLARITY_CASE(3);
		G3507_EXTI_POLARITY_CASE(4);
		G3507_EXTI_POLARITY_CASE(5);
		G3507_EXTI_POLARITY_CASE(6);
		G3507_EXTI_POLARITY_CASE(7);
		G3507_EXTI_POLARITY_CASE(8);
		G3507_EXTI_POLARITY_CASE(9);
		G3507_EXTI_POLARITY_CASE(10);
		G3507_EXTI_POLARITY_CASE(11);
		G3507_EXTI_POLARITY_CASE(12);
		G3507_EXTI_POLARITY_CASE(13);
		G3507_EXTI_POLARITY_CASE(14);
		G3507_EXTI_POLARITY_CASE(15);
		G3507_EXTI_POLARITY_CASE(16);
		G3507_EXTI_POLARITY_CASE(17);
		G3507_EXTI_POLARITY_CASE(18);
		G3507_EXTI_POLARITY_CASE(19);
		G3507_EXTI_POLARITY_CASE(20);
		G3507_EXTI_POLARITY_CASE(21);
		G3507_EXTI_POLARITY_CASE(22);
		G3507_EXTI_POLARITY_CASE(23);
		G3507_EXTI_POLARITY_CASE(24);
		G3507_EXTI_POLARITY_CASE(25);
		G3507_EXTI_POLARITY_CASE(26);
		G3507_EXTI_POLARITY_CASE(27);
		G3507_EXTI_POLARITY_CASE(28);
		G3507_EXTI_POLARITY_CASE(29);
		G3507_EXTI_POLARITY_CASE(30);
		G3507_EXTI_POLARITY_CASE(31);
		default:
			break;
	}

	return 0U;
}

void G3507_EXTI_Init(void *port, uint32_t pin, uint32_t trigger,
	uint32_t irqn, uint8_t preemptPriority, uint8_t subPriority)
{
	uint32_t polarity;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	API_GPIO_InitInputPullUp(port, pin);
	polarity = G3507_EXTI_GetPolarity(pin, trigger);
	if (pin <= 0x0000FFFFUL)
	{
		DL_GPIO_setLowerPinsPolarity((GPIO_Regs *)port, polarity);
	}
	else
	{
		DL_GPIO_setUpperPinsPolarity((GPIO_Regs *)port, polarity);
	}

	DL_GPIO_clearInterruptStatus((GPIO_Regs *)port, pin);
	DL_GPIO_enableInterrupt((GPIO_Regs *)port, pin);
	NVIC_ClearPendingIRQ((IRQn_Type)irqn);
	NVIC_SetPriority((IRQn_Type)irqn, preemptPriority);
	NVIC_EnableIRQ((IRQn_Type)irqn);
	(void)subPriority;
}

uint8_t G3507_EXTI_IsPendingAndClear(void *port, uint32_t pin)
{
	if ((port == 0) || (pin == 0U))
	{
		return 0U;
	}

	if (DL_GPIO_getEnabledInterruptStatus((GPIO_Regs *)port, pin) == 0U)
	{
		return 0U;
	}

	DL_GPIO_clearInterruptStatus((GPIO_Regs *)port, pin);
	return 1U;
}

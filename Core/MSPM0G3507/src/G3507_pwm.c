#include "G3507_pwm.h"

#include "G3507_hw_config.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_timera.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

typedef struct
{
	GPTIMER_Regs *regs;
} G3507_PWM_Map_t;

#define G3507_PWM_POWER_STARTUP_DELAY 16U

static void G3507_PWM_PowerStartupDelay(void)
{
	volatile uint32_t cycles;

	for (cycles = 0U; cycles < G3507_PWM_POWER_STARTUP_DELAY; ++cycles)
	{
		__NOP();
	}
}

static G3507_PWM_Map_t G3507_PWM_GetMap(uint8_t coreTimId)
{
	G3507_PWM_Map_t map;

	map.regs = 0;

	switch (coreTimId)
	{
	case API_PWM_CORE_TIMA0:
		map.regs = TIMA0;
		break;
	case API_PWM_CORE_TIMA1:
		map.regs = TIMA1;
		break;
	default:
		break;
	}

	return map;
}

static uint8_t G3507_PWM_GetClockDivider(uint32_t divisor, DL_TIMER_CLOCK_DIVIDE *divideRatio, uint8_t *prescale)
{
	static const uint8_t s_dividers[] = {8U, 4U, 2U, 1U};
	uint8_t i;

	if ((divideRatio == 0) || (prescale == 0) || (divisor == 0U))
	{
		return 0U;
	}

	for (i = 0U; i < (uint8_t)(sizeof(s_dividers) / sizeof(s_dividers[0])); ++i)
	{
		uint32_t ratio;
		uint32_t scaledPrescale;

		ratio = s_dividers[i];
		if ((divisor % ratio) != 0U)
		{
			continue;
		}

		scaledPrescale = divisor / ratio;
		if ((scaledPrescale == 0U) || (scaledPrescale > 256U))
		{
			continue;
		}

		switch (ratio)
		{
		case 8U:
			*divideRatio = DL_TIMER_CLOCK_DIVIDE_8;
			break;
		case 4U:
			*divideRatio = DL_TIMER_CLOCK_DIVIDE_4;
			break;
		case 2U:
			*divideRatio = DL_TIMER_CLOCK_DIVIDE_2;
			break;
		default:
			*divideRatio = DL_TIMER_CLOCK_DIVIDE_1;
			break;
		}

		*prescale = (uint8_t)(scaledPrescale - 1U);
		return 1U;
	}

	return 0U;
}

void G3507_PWM_ConfigPin(uint8_t coreTimId, uint8_t coreChannel)
{
	if (coreTimId != API_PWM_CORE_TIMA1)
	{
		return;
	}

	if (coreChannel == API_PWM_CORE_CCP1)
	{
		DL_GPIO_initPeripheralOutputFunction(G3507_PWM_TIMA1_CH1_IOMUX, G3507_PWM_TIMA1_CH1_FUNC);
		DL_GPIO_enableOutput(G3507_PWM_TIMA1_CH1_PORT, G3507_PWM_TIMA1_CH1_PIN);
	}
	else if (coreChannel == API_PWM_CORE_CCP0)
	{
		DL_GPIO_initPeripheralOutputFunction(G3507_PWM_TIMA1_CH0_IOMUX, G3507_PWM_TIMA1_CH0_FUNC);
		DL_GPIO_enableOutput(G3507_PWM_TIMA1_CH0_PORT, G3507_PWM_TIMA1_CH0_PIN);
	}
}

void G3507_PWM_InitTimer(uint8_t coreTimId, uint16_t arr, uint16_t psc)
{
	G3507_PWM_Map_t map;
	DL_TimerA_ClockConfig clockConfig;
	DL_TimerA_PWMConfig pwmConfig;
	uint32_t divisor;

	map = G3507_PWM_GetMap(coreTimId);
	if (map.regs == 0)
	{
		return;
	}

	divisor = (uint32_t)psc + 1UL;
	if (G3507_PWM_GetClockDivider(divisor, &clockConfig.divideRatio, &clockConfig.prescale) == 0U)
	{
		return;
	}

	DL_TimerA_reset(map.regs);
	if (!DL_TimerA_isPowerEnabled(map.regs))
	{
		DL_TimerA_enablePower(map.regs);
		G3507_PWM_PowerStartupDelay();
		while (!DL_TimerA_isPowerEnabled(map.regs))
		{
		}
	}

	clockConfig.clockSel = DL_TIMER_CLOCK_BUSCLK;
	DL_TimerA_setClockConfig(map.regs, &clockConfig);

	pwmConfig.period = (uint32_t)arr + 1UL;
	pwmConfig.pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN;
	pwmConfig.isTimerWithFourCC = false;
	pwmConfig.startTimer = DL_TIMER_STOP;
	DL_TimerA_setLoadValue(map.regs, pwmConfig.period - 1UL);
	DL_TimerA_setCaptureCompareAction(map.regs, (DL_TIMER_CC_LACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW), DL_TIMER_CC_0_INDEX);
	DL_TimerA_setCaptureCompareAction(map.regs, (DL_TIMER_CC_LACT_CCP_HIGH | DL_TIMER_CC_CDACT_CCP_LOW), DL_TIMER_CC_1_INDEX);
	DL_TimerA_setCaptureCompareCtl(map.regs, DL_TIMER_CC_MODE_COMPARE, 0U, DL_TIMER_CC_0_INDEX);
	DL_TimerA_setCaptureCompareCtl(map.regs, DL_TIMER_CC_MODE_COMPARE, 0U, DL_TIMER_CC_1_INDEX);
	DL_TimerA_setCaptureCompareInput(map.regs, DL_TIMER_CC_INPUT_INV_NOINVERT, DL_TIMER_CC_IN_SEL_CCPX, DL_TIMER_CC_0_INDEX);
	DL_TimerA_setCaptureCompareInput(map.regs, DL_TIMER_CC_INPUT_INV_NOINVERT, DL_TIMER_CC_IN_SEL_CCPX, DL_TIMER_CC_1_INDEX);
	DL_TimerA_setCounterRepeatMode(map.regs, DL_TIMER_REPEAT_MODE_ENABLED);
	DL_TimerA_setCounterValueAfterEnable(map.regs, DL_TIMER_COUNT_AFTER_EN_LOAD_VAL);
	DL_TimerA_setCounterControl(map.regs, DL_TIMER_CZC_CCCTL0_ZCOND, DL_TIMER_CAC_CCCTL0_ACOND, DL_TIMER_CLC_CCCTL0_LCOND);
	DL_TimerA_setCCPDirection(map.regs, (DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT));
	DL_TimerA_setCCPOutputDisabled(map.regs, DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL, DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL);
	DL_TimerA_setCaptureCompareOutCtl(map.regs, DL_TIMER_CC_OCTL_INIT_VAL_LOW, DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMER_CC_0_INDEX);
	DL_TimerA_setCaptureCompareOutCtl(map.regs, DL_TIMER_CC_OCTL_INIT_VAL_LOW, DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMER_CC_1_INDEX);
	DL_TimerA_setCaptCompUpdateMethod(map.regs, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMER_CC_0_INDEX);
	DL_TimerA_setCaptCompUpdateMethod(map.regs, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMER_CC_1_INDEX);
	DL_TimerA_setTimerCount(map.regs, 0U);
	DL_TimerA_setCaptureCompareValue(map.regs, 0U, DL_TIMER_CC_0_INDEX);
	DL_TimerA_setCaptureCompareValue(map.regs, 0U, DL_TIMER_CC_1_INDEX);
	DL_TimerA_enableClock(map.regs);
	DL_TimerA_startCounter(map.regs);
}

void G3507_PWM_SetCCR(uint8_t coreTimId, uint8_t coreChannel, uint16_t ccr)
{
	G3507_PWM_Map_t map;
	DL_TIMER_CC_INDEX ccIndex;
	uint32_t period;
	uint32_t duty;
	uint32_t compareValue;

	map = G3507_PWM_GetMap(coreTimId);
	if (map.regs == 0)
	{
		return;
	}

	if (coreChannel == API_PWM_CORE_CCP0)
	{
		ccIndex = DL_TIMER_CC_0_INDEX;
	}
	else if (coreChannel == API_PWM_CORE_CCP1)
	{
		ccIndex = DL_TIMER_CC_1_INDEX;
	}
	else
	{
		return;
	}

	period = DL_Timer_getLoadValue(map.regs) + 1UL;
	duty = (uint32_t)ccr;
	if (duty > period)
	{
		duty = period;
	}

	/* Edge-aligned down-count mode: CCP high width = period - compare, so convert duty to compare. */
	compareValue = period - duty;
	DL_TimerA_setCaptureCompareValue(map.regs, compareValue, ccIndex);
}

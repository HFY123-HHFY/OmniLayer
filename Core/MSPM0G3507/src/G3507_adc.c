#include "G3507_adc.h"

#include "G3507_gpio.h"

#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

#define G3507_ADC0_BASE ADC0
#define G3507_ADC1_BASE ADC1

#define G3507_ADC_MAX    2U
#define G3507_ADC_CH_MAX  19U

#define G3507_ADC_CONVERSION_TIMEOUT 1000000UL
#define G3507_ADC_POWERUP_TIMEOUT    1000000UL

typedef struct
{
	void *port;
	uint32_t pin;
	uint32_t iomux;
	uint8_t initialized;
	uint8_t channel;
} G3507_ADC_ChannelState_t;

static G3507_ADC_ChannelState_t s_adcState[G3507_ADC_MAX][G3507_ADC_CH_MAX] = {0};

static ADC12_Regs *G3507_ADC_GetBase(uint8_t adcId)
{
	switch (adcId)
	{
	case 0U:
		return G3507_ADC0_BASE;
	case 1U:
		return G3507_ADC1_BASE;
	default:
		return 0;
	}
}

static void G3507_ADC_EnsureAdcReady(ADC12_Regs *adcBase)
{
	DL_ADC12_ClockConfig adcClockConfig;
	uint32_t timeout;

	if (adcBase == 0)
	{
		return;
	}

	if (DL_ADC12_isPowerEnabled(adcBase))
	{
		return;
	}

	DL_ADC12_reset(adcBase);
	DL_ADC12_enablePower(adcBase);
	timeout = G3507_ADC_POWERUP_TIMEOUT;
	while ((!DL_ADC12_isPowerEnabled(adcBase)) && (timeout > 0UL))
	{
		--timeout;
	}
	if (timeout == 0UL)
	{
		return;
	}

	adcClockConfig.clockSel = DL_ADC12_CLOCK_SYSOSC;
	adcClockConfig.freqRange = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32;
	adcClockConfig.divideRatio = DL_ADC12_CLOCK_DIVIDE_8;
	DL_ADC12_setClockConfig(adcBase, &adcClockConfig);

	DL_ADC12_disableConversions(adcBase);
	DL_ADC12_clearInterruptStatus(adcBase,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_initSingleSample(adcBase,
							 DL_ADC12_REPEAT_MODE_ENABLED,
							 DL_ADC12_SAMPLING_SOURCE_AUTO,
							 DL_ADC12_TRIG_SRC_SOFTWARE,
							 DL_ADC12_SAMP_CONV_RES_12_BIT,
							 DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
	DL_ADC12_setStartAddress(adcBase, DL_ADC12_SEQ_START_ADDR_00);
	DL_ADC12_setSampleTime0(adcBase, 40000U);
	DL_ADC12_setPowerDownMode(adcBase, DL_ADC12_POWER_DOWN_MODE_MANUAL);
	DL_ADC12_enableConversions(adcBase);
}

static uint8_t G3507_ADC_IsValidChannel(uint8_t adcId, uint8_t channel)
{
	return ((adcId < G3507_ADC_MAX) && (channel < G3507_ADC_CH_MAX)) ? 1U : 0U;
}

static uint8_t G3507_ADC_ApplyChannelConfig(uint8_t adcId, uint8_t channel)
{
	ADC12_Regs *adcBase;
	G3507_ADC_ChannelState_t *state;

	if (G3507_ADC_IsValidChannel(adcId, channel) == 0U)
	{
		return 0U;
	}

	state = &s_adcState[adcId][channel];
	if ((state->initialized == 0U) || (state->iomux == 0xFFFFFFFFUL))
	{
		return 0U;
	}

	adcBase = G3507_ADC_GetBase(adcId);
	if (adcBase == 0)
	{
		return 0U;
	}

	DL_ADC12_disableConversions(adcBase);
	DL_ADC12_clearInterruptStatus(adcBase,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_configConversionMem(adcBase,
							   DL_ADC12_MEM_IDX_0,
							   (uint32_t)state->channel,
							   DL_ADC12_REFERENCE_VOLTAGE_VDDA,
							   DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
							   DL_ADC12_AVERAGING_MODE_DISABLED,
							   DL_ADC12_BURN_OUT_SOURCE_DISABLED,
							   DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
							   DL_ADC12_WINDOWS_COMP_MODE_DISABLED);

	return 1U;
}

void G3507_ADC_InitChannel(uint8_t adcId, uint8_t channel, void *port, uint32_t pin)
{
	ADC12_Regs *adcBase;
	uint32_t iomux;
	GPIO_Regs *gpioPort;
	G3507_ADC_ChannelState_t *state;

	if (G3507_ADC_IsValidChannel(adcId, channel) == 0U)
	{
		return;
	}

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	adcBase = G3507_ADC_GetBase(adcId);
	if (adcBase == 0)
	{
		return;
	}

	iomux = G3507_GetIomux(port, pin);
	if (iomux == 0xFFFFFFFFUL)
	{
		return;
	}

	gpioPort = (GPIO_Regs *)port;
	if (!DL_GPIO_isPowerEnabled(gpioPort))
	{
		DL_GPIO_reset(gpioPort);
		DL_GPIO_enablePower(gpioPort);
		while (!DL_GPIO_isPowerEnabled(gpioPort))
		{
		}
	}

	DL_GPIO_initPeripheralAnalogFunction(iomux);
	state = &s_adcState[adcId][channel];
	state->port = port;
	state->pin = pin;
	state->iomux = iomux;
	state->channel = channel;
	state->initialized = 1U;

	G3507_ADC_EnsureAdcReady(adcBase);
	if (!DL_ADC12_isPowerEnabled(adcBase))
	{
		state->initialized = 0U;
		return;
	}

	DL_ADC12_disableConversions(adcBase);
	DL_ADC12_clearInterruptStatus(adcBase,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_configConversionMem(adcBase,
							   DL_ADC12_MEM_IDX_0,
							   channel,
							   DL_ADC12_REFERENCE_VOLTAGE_VDDA,
							   DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
							   DL_ADC12_AVERAGING_MODE_DISABLED,
							   DL_ADC12_BURN_OUT_SOURCE_DISABLED,
							   DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
							   DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
}

uint16_t G3507_ADC_ReadChannel(uint8_t adcId, uint8_t channel)
{
	ADC12_Regs *adcBase;
	uint32_t timeout;
	G3507_ADC_ChannelState_t *state;

	if (G3507_ADC_IsValidChannel(adcId, channel) == 0U)
	{
		return 0U;
	}

	state = &s_adcState[adcId][channel];
	if ((state->initialized == 0U) || (state->iomux == 0xFFFFFFFFUL))
	{
		return 0U;
	}

	adcBase = G3507_ADC_GetBase(adcId);
	if (adcBase == 0)
	{
		return 0U;
	}

	G3507_ADC_EnsureAdcReady(adcBase);
	if (!DL_ADC12_isPowerEnabled(adcBase))
	{
		return 0U;
	}

	if (G3507_ADC_ApplyChannelConfig(adcId, channel) == 0U)
	{
		return 0U;
	}

	if (!DL_ADC12_isConversionsEnabled(adcBase))
	{
		DL_ADC12_enableConversions(adcBase);
		if (!DL_ADC12_isConversionsEnabled(adcBase))
		{
			return 0U;
		}
	}

	DL_ADC12_clearInterruptStatus(adcBase,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_startConversion(adcBase);

	timeout = G3507_ADC_CONVERSION_TIMEOUT;
	while ((DL_ADC12_getRawInterruptStatus(adcBase, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) && (timeout > 0UL))
	{
		--timeout;
	}

	if (timeout == 0UL)
	{
		return 0U;
	}

	return (uint16_t)DL_ADC12_getMemResult(adcBase, DL_ADC12_MEM_IDX_0);
}

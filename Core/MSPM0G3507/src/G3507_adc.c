#include "G3507_adc.h"

#include "G3507_gpio.h"

#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

#define G3507_ADC0_BASE ADC0

#define G3507_ADC_MAX    1U
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

static void G3507_ADC_EnsureAdcReady(void)
{
	DL_ADC12_ClockConfig adcClockConfig;
	uint32_t timeout;

	if (DL_ADC12_isPowerEnabled(G3507_ADC0_BASE))
	{
		return;
	}

	DL_ADC12_reset(G3507_ADC0_BASE);
	DL_ADC12_enablePower(G3507_ADC0_BASE);
	timeout = G3507_ADC_POWERUP_TIMEOUT;
	while ((!DL_ADC12_isPowerEnabled(G3507_ADC0_BASE)) && (timeout > 0UL))
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
	DL_ADC12_setClockConfig(G3507_ADC0_BASE, &adcClockConfig);

	DL_ADC12_disableConversions(G3507_ADC0_BASE);
	DL_ADC12_clearInterruptStatus(G3507_ADC0_BASE,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_initSingleSample(G3507_ADC0_BASE,
							 DL_ADC12_REPEAT_MODE_ENABLED,
							 DL_ADC12_SAMPLING_SOURCE_AUTO,
							 DL_ADC12_TRIG_SRC_SOFTWARE,
							 DL_ADC12_SAMP_CONV_RES_12_BIT,
							 DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
	DL_ADC12_setStartAddress(G3507_ADC0_BASE, DL_ADC12_SEQ_START_ADDR_00);
	DL_ADC12_setSampleTime0(G3507_ADC0_BASE, 40000U);
	DL_ADC12_setPowerDownMode(G3507_ADC0_BASE, DL_ADC12_POWER_DOWN_MODE_MANUAL);
	DL_ADC12_enableConversions(G3507_ADC0_BASE);
}

static uint8_t G3507_ADC_IsValidChannel(uint8_t adcId, uint8_t channel)
{
	return ((adcId < G3507_ADC_MAX) && (channel < G3507_ADC_CH_MAX)) ? 1U : 0U;
}

static uint8_t G3507_ADC_ApplyChannelConfig(uint8_t adcId, uint8_t channel)
{
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

	DL_ADC12_disableConversions(G3507_ADC0_BASE);
	DL_ADC12_clearInterruptStatus(G3507_ADC0_BASE,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_configConversionMem(G3507_ADC0_BASE,
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
	uint32_t iomux;
	GPIO_Regs *gpioPort;
	G3507_ADC_ChannelState_t *state;

	if (G3507_ADC_IsValidChannel(adcId, channel) == 0U)
	{
		return;
	}

	if ((port == NULL) || (pin == 0U))
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
	G3507_ADC_EnsureAdcReady();
	if (!DL_ADC12_isPowerEnabled(G3507_ADC0_BASE))
	{
		return;
	}

	// 直接在初始化时应用通道配置
	DL_ADC12_disableConversions(G3507_ADC0_BASE);
	DL_ADC12_clearInterruptStatus(G3507_ADC0_BASE,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_configConversionMem(G3507_ADC0_BASE,
							   DL_ADC12_MEM_IDX_0,
							   channel,
							   DL_ADC12_REFERENCE_VOLTAGE_VDDA,
							   DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
							   DL_ADC12_AVERAGING_MODE_DISABLED,
							   DL_ADC12_BURN_OUT_SOURCE_DISABLED,
							   DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
							   DL_ADC12_WINDOWS_COMP_MODE_DISABLED);

	state = &s_adcState[adcId][channel];
	state->port = port;
	state->pin = pin;
	state->iomux = iomux;
	state->channel = channel;
	state->initialized = 1U;
}

uint16_t G3507_ADC_ReadChannel(uint8_t adcId, uint8_t channel)
{
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

	G3507_ADC_EnsureAdcReady();
	if (!DL_ADC12_isPowerEnabled(G3507_ADC0_BASE))
	{
		return 0U;
	}

	if (G3507_ADC_ApplyChannelConfig(adcId, channel) == 0U)
	{
		return 0U;
	}

	if (!DL_ADC12_isConversionsEnabled(G3507_ADC0_BASE))
	{
		DL_ADC12_enableConversions(G3507_ADC0_BASE);
		if (!DL_ADC12_isConversionsEnabled(G3507_ADC0_BASE))
		{
			return 0U;
		}
	}

	DL_ADC12_clearInterruptStatus(G3507_ADC0_BASE,
		DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED |
		DL_ADC12_INTERRUPT_OVERFLOW |
		DL_ADC12_INTERRUPT_UNDERFLOW);
	DL_ADC12_startConversion(G3507_ADC0_BASE);

	timeout = G3507_ADC_CONVERSION_TIMEOUT;
	while ((DL_ADC12_getRawInterruptStatus(G3507_ADC0_BASE, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) && (timeout > 0UL))
	{
		--timeout;
	}

	if (timeout == 0UL)
	{
		return 0U;
	}

	return (uint16_t)DL_ADC12_getMemResult(G3507_ADC0_BASE, DL_ADC12_MEM_IDX_0);
}

#include "G3507_adc.h"

#include "G3507_hw_config.h"
#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

/* 只支持 ADC0 A25 输入（板级逻辑通道 API_ADC_CH2） */
#define G3507_ADC0_BASE ADC0
#define G3507_ADC0_INPUT_CH DL_ADC12_INPUT_CHAN_2

static uint8_t s_adc0_ch2_inited = 0U;

#define G3507_ADC_CONVERSION_TIMEOUT 1000000UL
#define G3507_ADC_POWERUP_TIMEOUT    1000000UL

void G3507_ADC_InitChannel(uint8_t adcId, uint8_t channel, void *port, uint32_t pin)
{
	DL_ADC12_ClockConfig adcClockConfig;
	uint32_t timeout;

	(void)port;
	(void)pin;
	if (adcId == 0U && channel == 2U && !s_adc0_ch2_inited) {
		DL_GPIO_initPeripheralAnalogFunction(G3507_ADC0_CH2_IOMUX);
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
		DL_ADC12_configConversionMem(G3507_ADC0_BASE,
		                           DL_ADC12_MEM_IDX_0,
		                           G3507_ADC0_INPUT_CH,
		                           DL_ADC12_REFERENCE_VOLTAGE_VDDA,
		                           DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
		                           DL_ADC12_AVERAGING_MODE_DISABLED,
		                           DL_ADC12_BURN_OUT_SOURCE_DISABLED,
		                           DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
		                           DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
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
		s_adc0_ch2_inited = 1U;
	}
}

uint16_t G3507_ADC_ReadChannel(uint8_t adcId, uint8_t channel)
{
	uint32_t timeout;

	if (adcId == 0U && channel == 2U && s_adc0_ch2_inited) {
		if (!DL_ADC12_isConversionsEnabled(G3507_ADC0_BASE))
		{
			return 0U;
		}

		DL_ADC12_clearInterruptStatus(G3507_ADC0_BASE, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
		DL_ADC12_startConversion(G3507_ADC0_BASE);

		timeout = G3507_ADC_CONVERSION_TIMEOUT;
		while (((DL_ADC12_getRawInterruptStatus(G3507_ADC0_BASE, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED)) == 0U) && (timeout > 0UL))
		{
			--timeout;
		}

		if (timeout == 0UL)
		{
			return 0U;
		}

		return (uint16_t)DL_ADC12_getMemResult(G3507_ADC0_BASE, DL_ADC12_MEM_IDX_0);
	}
	return 0U;
}

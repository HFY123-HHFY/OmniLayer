#include "adc.h"

/* 注册层下发的 ADC 板级映射表。 */
static const API_ADC_Config_t *s_adcTable = 0;
/* 当前映射表项数量。 */
static uint8_t s_adcCount = 0U;

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
static void API_ADC_CoreInitChannel(API_ADC_Id_t id, uint8_t channel, void *port, uint16_t pin)
{
	F103_ADC_InitChannel((uint8_t)id, channel, port, pin);
}

static uint16_t API_ADC_CoreReadChannel(API_ADC_Id_t id, uint8_t channel)
{
	return F103_ADC_ReadChannel((uint8_t)id, channel);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
static void API_ADC_CoreInitChannel(API_ADC_Id_t id, uint8_t channel, void *port, uint16_t pin)
{
	F407_ADC_InitChannel((uint8_t)id, channel, port, pin);
}

static uint16_t API_ADC_CoreReadChannel(API_ADC_Id_t id, uint8_t channel)
{
	return F407_ADC_ReadChannel((uint8_t)id, channel);
}
#else
static void API_ADC_CoreInitChannel(API_ADC_Id_t id, uint8_t channel, void *port, uint16_t pin)
{
	(void)id;
	(void)channel;
	(void)port;
	(void)pin;
}

static uint16_t API_ADC_CoreReadChannel(API_ADC_Id_t id, uint8_t channel)
{
	(void)id;
	(void)channel;
	return 0U;
}
#endif

/* 校验 ADC 实例号是否在 API 范围内。 */
static uint8_t API_ADC_IsValidId(API_ADC_Id_t id)
{
	return ((uint32_t)id <= (uint32_t)API_ADC3) ? 1U : 0U;
}

/* 校验 ADC 实例号是否在 API 范围内。 */
static uint8_t API_ADC_IsMappedId(API_ADC_Id_t id)
{
	uint8_t i;

	if ((s_adcTable == 0) || (s_adcCount == 0U))
	{
		return 0U;
	}

	for (i = 0U; i < s_adcCount; ++i)
	{
		if (s_adcTable[i].adcId == id)
		{
			return 1U;
		}
	}

	return 0U;
}

/* 校验 ADC+通道是否在板级映射表中登记。 */
static uint8_t API_ADC_IsMappedChannel(API_ADC_Id_t id, API_ADC_Channel_t channel)
{
	uint8_t i;

	if ((s_adcTable == 0) || (s_adcCount == 0U))
	{
		return 0U;
	}

	for (i = 0U; i < s_adcCount; ++i)
	{
		if ((s_adcTable[i].adcId == id) && (s_adcTable[i].channel == channel))
		{
			return 1U;
		}
	}

	return 0U;
}

/* 注册板级 ADC 映射。 */
void API_ADC_Register(const API_ADC_Config_t *configTable, uint8_t count)
{
	s_adcTable = configTable;
	s_adcCount = count;
}

/* ADC 初始化函数：x 为要初始化的 ADC 几。 */
void API_ADC_Init(API_ADC_Id_t x)
{
	uint8_t i;

	if (API_ADC_IsValidId(x) == 0U)
	{
		return;
	}

	if (API_ADC_IsMappedId(x) == 0U)
	{
		return;
	}

	/* 按板级映射表初始化该 ADC 下的所有通道引脚。 */
	for (i = 0U; i < s_adcCount; ++i)
	{
		if (s_adcTable[i].adcId == x)
		{
			API_ADC_CoreInitChannel(s_adcTable[i].adcId,
			                        s_adcTable[i].channel,
			                        s_adcTable[i].port,
			                        s_adcTable[i].pin);
		}
	}
}

/* ADC 获取函数：x 选择 ADC 几，y 选择该 ADC 的通道号。 */
uint16_t API_ADC_GetValue(API_ADC_Id_t x, API_ADC_Channel_t y)
{
	if (API_ADC_IsValidId(x) == 0U)
	{
		return 0U;
	}

	if (API_ADC_IsMappedChannel(x, y) == 0U)
	{
		return 0U;
	}

	return API_ADC_CoreReadChannel(x, y);
}

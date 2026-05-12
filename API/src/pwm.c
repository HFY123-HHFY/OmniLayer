#include "pwm.h"

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "103_hw_config.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "407_hw_config.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#include "G3507_hw_config.h"
#endif

/* 注册层下发的 PWM 引脚映射表。 */
static const API_PWM_Config_t *s_pwmTable = 0;
/* 当前映射表项数量。 */
static uint8_t s_pwmCount = 0U;

#define API_PWM_ITEM(timId, channel, coreTimId, coreChannel, port, pin) \
	{ timId, channel, coreTimId, coreChannel, port, pin },

static const API_PWM_Config_t s_defaultPwmTable[] =
{
	HW_PWM_MAP(API_PWM_ITEM)
};

#undef API_PWM_ITEM

static void API_PWM_EnsureRegistered(void)
{
	if ((s_pwmTable == 0) || (s_pwmCount == 0U))
	{
		API_PWM_Register(s_defaultPwmTable, HW_PWM_COUNT);
	}
}

/* 查找逻辑 PWM 定时器绑定的第一项配置。 */
static const API_PWM_Config_t *API_PWM_FindTimerConfig(API_PWM_Tim_t timId)
{
	uint8_t i;

	API_PWM_EnsureRegistered();

	if ((s_pwmTable == 0) || (s_pwmCount == 0U))
	{
		return 0;
	}

	for (i = 0U; i < s_pwmCount; ++i)
	{
		if (s_pwmTable[i].timId == timId)
		{
			return &s_pwmTable[i];
		}
	}

	return 0;
}

/* 查找逻辑 PWM 通道绑定的配置。 */
static const API_PWM_Config_t *API_PWM_FindChannelConfig(API_PWM_Tim_t timId, API_PWM_Channel_t channel)
{
	uint8_t i;

	API_PWM_EnsureRegistered();

	if ((s_pwmTable == 0) || (s_pwmCount == 0U))
	{
		return 0;
	}

	for (i = 0U; i < s_pwmCount; ++i)
	{
		if ((s_pwmTable[i].timId == timId) && (s_pwmTable[i].channel == channel))
		{
			return &s_pwmTable[i];
		}
	}

	return 0;
}

/* 保存板级 PWM 映射。 */
void API_PWM_Register(const API_PWM_Config_t *configTable, uint8_t count)
{
	s_pwmTable = configTable;
	s_pwmCount = count;
}

/* 检查定时器是否在映射表中登记。 */
static uint8_t API_PWM_HasTimer(uint8_t timId)
{
	uint8_t i;

	API_PWM_EnsureRegistered();

	if ((s_pwmTable == 0) || (s_pwmCount == 0U))
	{
		return 0U;
	}

	for (i = 0U; i < s_pwmCount; ++i)
	{
		if (s_pwmTable[i].timId == timId)
		{
			return 1U;
		}
	}

	return 0U;
}

/* 检查定时器通道是否在映射表中登记。 */
static uint8_t API_PWM_HasChannel(uint8_t timId, uint8_t channel)
{
	uint8_t i;

	API_PWM_EnsureRegistered();

	if ((s_pwmTable == 0) || (s_pwmCount == 0U))
	{
		return 0U;
	}

	for (i = 0U; i < s_pwmCount; ++i)
	{
		if ((s_pwmTable[i].timId == timId) && (s_pwmTable[i].channel == channel))
		{
			return 1U;
		}
	}

	return 0U;
}

void API_PWM_Init(API_PWM_Tim_t timId, uint16_t arr, uint16_t psc)
{
	uint8_t i;
	const API_PWM_Config_t *timerConfig;

	if (API_PWM_HasTimer(timId) == 0U)
	{
		return;
	}

	timerConfig = API_PWM_FindTimerConfig(timId);
	if (timerConfig == 0)
	{
		return;
	}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	/* 先按映射表配置该定时器对应的所有 PWM 引脚，再启动定时器基准。 */
	for (i = 0U; i < s_pwmCount; ++i)
	{
		if (s_pwmTable[i].timId == timId)
		{
			F103_PWM_ConfigPin(s_pwmTable[i].port, s_pwmTable[i].pin);
		}
	}
	F103_PWM_InitTimer(timerConfig->coreTimId, arr, psc);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	/* 先按映射表配置该定时器对应的所有 PWM 引脚，再启动定时器基准。 */
	for (i = 0U; i < s_pwmCount; ++i)
	{
		if (s_pwmTable[i].timId == timId)
		{
			F407_PWM_ConfigPin(s_pwmTable[i].port, s_pwmTable[i].pin, s_pwmTable[i].coreTimId);
		}
	}
	F407_PWM_InitTimer(timerConfig->coreTimId, arr, psc);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	for (i = 0U; i < s_pwmCount; ++i)
	{
		if (s_pwmTable[i].timId == timId)
		{
			G3507_PWM_ConfigPin(s_pwmTable[i].coreTimId, s_pwmTable[i].coreChannel);
		}
	}
	G3507_PWM_InitTimer(timerConfig->coreTimId, arr, psc);
#else
	(void)timId;
	(void)arr;
	(void)psc;
#endif
}

void API_PWM_Setcom(API_PWM_Tim_t timId, API_PWM_Channel_t channel, uint16_t ccr)
{
	const API_PWM_Config_t *config;

	if (API_PWM_HasChannel(timId, channel) == 0U)
	{
		return;
	}

	config = API_PWM_FindChannelConfig(timId, channel);
	if (config == 0)
	{
		return;
	}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	F103_PWM_SetCCR(config->coreTimId, config->coreChannel, ccr);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	F407_PWM_SetCCR(config->coreTimId, config->coreChannel, ccr);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	G3507_PWM_SetCCR(config->coreTimId, config->coreChannel, ccr);
#else
	(void)timId;
	(void)channel;
	(void)ccr;
#endif
}

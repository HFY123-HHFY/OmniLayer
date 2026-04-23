#include "pwm.h"

/* 注册层下发的 PWM 引脚映射表。 */
static const API_PWM_Config_t *s_pwmTable = 0;
/* 当前映射表项数量。 */
static uint8_t s_pwmCount = 0U;

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

void API_PWM_Init(uint8_t timId, uint16_t arr, uint16_t psc)
{
	uint8_t i;

	if (API_PWM_HasTimer(timId) == 0U)
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
	F103_PWM_InitTimer(timId, arr, psc);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	/* 先按映射表配置该定时器对应的所有 PWM 引脚，再启动定时器基准。 */
	for (i = 0U; i < s_pwmCount; ++i)
	{
		if (s_pwmTable[i].timId == timId)
		{
			F407_PWM_ConfigPin(s_pwmTable[i].port, s_pwmTable[i].pin, timId);
		}
	}
	F407_PWM_InitTimer(timId, arr, psc);
#else
	(void)timId;
	(void)arr;
	(void)psc;
#endif
}

void API_PWM_Setcom(uint8_t timId, uint8_t channel, uint16_t ccr)
{
	if (API_PWM_HasChannel(timId, channel) == 0U)
	{
		return;
	}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	F103_PWM_SetCCR(timId, channel, ccr);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	F407_PWM_SetCCR(timId, channel, ccr);
#else
	(void)timId;
	(void)channel;
	(void)ccr;
#endif
}

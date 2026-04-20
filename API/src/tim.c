#include "tim.h"

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
static void API_TIM_CoreInit(API_TIM_Id_t id, uint32_t periodMs)
{
	F103_TIM_PeriodicInit((uint8_t)id, periodMs);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
static void API_TIM_CoreInit(API_TIM_Id_t id, uint32_t periodMs)
{
	F407_TIM_PeriodicInit((uint8_t)id, periodMs);
}
#else
static void API_TIM_CoreInit(API_TIM_Id_t id, uint32_t periodMs)
{
	(void)id;
	(void)periodMs;
}
#endif

/* 检查定时器 id 是否在 API 约定范围内。 */
static uint8_t API_TIM_IsValidId(API_TIM_Id_t id)
{
	return ((uint32_t)id <= (uint32_t)API_TIM5) ? 1U : 0U;
}

/* 定时器初始化入口：id 选定时器，periodMs 设置中断周期。 */
void API_TIM_Init(API_TIM_Id_t id, uint32_t periodMs)
{
	if ((API_TIM_IsValidId(id) == 0U) || (periodMs == 0U))
	{
		return;
	}

	API_TIM_CoreInit(id, periodMs);
}

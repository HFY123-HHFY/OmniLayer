#include "tim.h"

#define API_TIM_MAX_ID  ((uint8_t)API_TIM5)

static const API_TIM_Config_t *s_timTable;
static uint8_t s_timCount;
static API_TIM_IrqHandler_t s_timIrqHandlers[API_TIM_MAX_ID + 1U];
static uint8_t s_timStarted[API_TIM_MAX_ID + 1U];

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
static void API_TIM_CoreInit(uint8_t coreId, uint32_t periodMs)
{
	F103_TIM_PeriodicInit(coreId, periodMs);
}

static uint8_t API_TIM_CoreCheckAndClearIrq(uint8_t coreId)
{
	return F103_TIM_CheckAndClearUpdateIrq(coreId);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
static void API_TIM_CoreInit(uint8_t coreId, uint32_t periodMs)
{
	F407_TIM_PeriodicInit(coreId, periodMs);
}

static uint8_t API_TIM_CoreCheckAndClearIrq(uint8_t coreId)
{
	return F407_TIM_CheckAndClearUpdateIrq(coreId);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
static void API_TIM_CoreInit(uint8_t coreId, uint32_t periodMs)
{
	G3507_TIM_PeriodicInit(coreId, periodMs);
}

static uint8_t API_TIM_CoreCheckAndClearIrq(uint8_t coreId)
{
	return G3507_TIM_CheckAndClearUpdateIrq(coreId);
}
#else
static void API_TIM_CoreInit(uint8_t coreId, uint32_t periodMs)
{
	(void)coreId;
	(void)periodMs;
}

static uint8_t API_TIM_CoreCheckAndClearIrq(uint8_t coreId)
{
	(void)coreId;
	return 0U;
}
#endif

static uint8_t API_TIM_IsValidId(API_TIM_Id_t id)
{
	return ((uint8_t)id <= API_TIM_MAX_ID) ? 1U : 0U;
}

static const API_TIM_Config_t *API_TIM_FindConfigById(API_TIM_Id_t id)
{
	uint8_t i;

	if ((s_timTable == 0) || (s_timCount == 0U))
	{
		return 0;
	}

	for (i = 0U; i < s_timCount; ++i)
	{
		if (s_timTable[i].id == id)
		{
			return &s_timTable[i];
		}
	}

	return 0;
}

static const API_TIM_Config_t *API_TIM_FindConfigByCoreId(uint8_t coreId)
{
	uint8_t i;

	if ((s_timTable == 0) || (s_timCount == 0U))
	{
		return 0;
	}

	for (i = 0U; i < s_timCount; ++i)
	{
		if (s_timTable[i].coreId == coreId)
		{
			return &s_timTable[i];
		}
	}

	return 0;
}

void API_TIM_Register(const API_TIM_Config_t *configTable, uint8_t count)
{
	s_timTable = configTable;
	s_timCount = count;
}

void API_TIM_RegisterIrqHandler(API_TIM_Id_t id, API_TIM_IrqHandler_t handler)
{
	if (API_TIM_IsValidId(id) == 0U)
	{
		return;
	}

	s_timIrqHandlers[(uint8_t)id] = handler;
}

void API_TIM_Init(API_TIM_Id_t id, uint32_t periodMs)
{
	const API_TIM_Config_t *config;

	if ((API_TIM_IsValidId(id) == 0U) || (periodMs == 0U))
	{
		return;
	}

	config = API_TIM_FindConfigById(id);
	if (config == 0)
	{
		return;
	}

	API_TIM_CoreInit(config->coreId, periodMs);
	s_timStarted[(uint8_t)id] = 1U;
}

void API_TIM_HandleIrqByCoreId(uint8_t coreId)
{
	const API_TIM_Config_t *config;
	API_TIM_IrqHandler_t handler;

	config = API_TIM_FindConfigByCoreId(coreId);
	if (config == 0)
	{
		return;
	}

	if (s_timStarted[(uint8_t)config->id] == 0U)
	{
		return;
	}

	if (API_TIM_CoreCheckAndClearIrq(coreId) == 0U)
	{
		return;
	}

	handler = s_timIrqHandlers[(uint8_t)config->id];
	if (handler != 0)
	{
		handler(config->id);
	}
}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
void TIM2_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIM2);
}

void TIM3_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIM3);
}

void TIM4_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIM4);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
void TIM2_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIM2);
}

void TIM3_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIM3);
}

void TIM4_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIM4);
}

void TIM5_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIM5);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
void TIMG0_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIMG0);
}

void TIMG6_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIMG6);
}

void TIMA0_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIMA0);
}

void TIMA1_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIMA1);
}

void TIMG7_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIMG7);
}

void TIMG8_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIMG8);
}

void TIMG12_IRQHandler(void)
{
	API_TIM_HandleIrqByCoreId(API_TIM_CORE_TIMG12);
}
#endif

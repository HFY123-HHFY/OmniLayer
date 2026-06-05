#include "Encoder.h"

/*
 * API Encoder 层实现：
 * - 条件编译分发到 Core 层实现；
 * - 维护编码器配置表，供 Init 时使用。
 */

/* 编码器速度值 */
int16_t Encoder1_Speed = 0;
int16_t Encoder2_Speed = 0;

#define API_ENCODER_MAX_ID  ((uint8_t)API_ENCODER_2)

static const API_Encoder_Config_t *s_encoderTable;
static uint8_t                      s_encoderCount;
static uint8_t                      s_encoderInited[API_ENCODER_MAX_ID + 1U];

/*
 * Core 层条件编译分发
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
static void API_Encoder_CoreInit(uint8_t coreId,
                                 void *portA, uint32_t pinA,
                                 void *portB, uint32_t pinB)
{
	F103_Encoder_Init(coreId, portA, pinA, portB, pinB);
}

static int16_t API_Encoder_CoreGetCount(uint8_t coreId)
{
	return F103_Encoder_GetCount(coreId);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
static void API_Encoder_CoreInit(uint8_t coreId,
                                 void *portA, uint32_t pinA,
                                 void *portB, uint32_t pinB)
{
	F407_Encoder_Init(coreId, portA, pinA, portB, pinB);
}

static int16_t API_Encoder_CoreGetCount(uint8_t coreId)
{
	return F407_Encoder_GetCount(coreId);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
static void API_Encoder_CoreInit(uint8_t coreId,
                                 void *portA, uint32_t pinA,
                                 void *portB, uint32_t pinB)
{
	G3507_Encoder_SetPins(coreId, portA, pinA, portB, pinB);
	G3507_Encoder_Init(coreId);
}

static int16_t API_Encoder_CoreGetCount(uint8_t coreId)
{
	return G3507_Encoder_GetCount(coreId);
}
#else
static void API_Encoder_CoreInit(uint8_t coreId,
                                 void *portA, uint32_t pinA,
                                 void *portB, uint32_t pinB)
{
	(void)coreId;
	(void)portA;
	(void)pinA;
	(void)portB;
	(void)pinB;
}

static int16_t API_Encoder_CoreGetCount(uint8_t coreId)
{
	(void)coreId;
	return 0;
}
#endif

static uint8_t API_Encoder_IsValidId(API_Encoder_Id_t id)
{
	return ((uint8_t)id <= API_ENCODER_MAX_ID) ? 1U : 0U;
}

static const API_Encoder_Config_t *API_Encoder_FindConfigById(API_Encoder_Id_t id)
{
	uint8_t i;

	if ((s_encoderTable == 0) || (s_encoderCount == 0U))
	{
		return 0;
	}

	for (i = 0U; i < s_encoderCount; ++i)
	{
		if (s_encoderTable[i].id == id)
		{
			return &s_encoderTable[i];
		}
	}

	return 0;
}

void API_Encoder_Register(const API_Encoder_Config_t *configTable, uint8_t count)
{
	s_encoderTable = configTable;
	s_encoderCount = count;
}

void API_Encoder_Init(API_Encoder_Id_t id)
{
	const API_Encoder_Config_t *config;

	if (API_Encoder_IsValidId(id) == 0U)
	{
		return;
	}

	config = API_Encoder_FindConfigById(id);
	if (config == 0)
	{
		return;
	}

	API_Encoder_CoreInit(config->coreId,
	                     config->portA, config->pinA,
	                     config->portB, config->pinB);

	s_encoderInited[(uint8_t)id] = 1U;
}

int16_t API_Encoder_GetSpeed(API_Encoder_Id_t id)
{
	const API_Encoder_Config_t *config;

	if (API_Encoder_IsValidId(id) == 0U)
	{
		return 0;
	}

	if (s_encoderInited[(uint8_t)id] == 0U)
	{
		return 0;
	}

	config = API_Encoder_FindConfigById(id);
	if (config == 0)
	{
		return 0;
	}

	return API_Encoder_CoreGetCount(config->coreId);
}

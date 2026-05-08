#include "LED.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

/* s_ledConfigTable: 已注册 LED 配置表，由 Enroll 层通过 LED_Register 注入。 */
static const LED_Config_t *s_ledConfigTable;
/* s_ledConfigCount: 当前配置表中有效 LED 数量。 */
static uint8_t s_ledConfigCount;

/* 每个 LED 的闪烁上下文。 */
typedef struct
{
	LED_Level_t currentLevel;
	uint8_t levelKnown;
} LED_TurnContext_t;

static LED_TurnContext_t s_ledTurnCtx[3];

/* 归一化电平输入，保证只返回 LED_HIGH 或 LED_LOW。 */
static LED_Level_t LED_NormalizeLevel(LED_Level_t level)
{
	return (level == LED_HIGH) ? LED_HIGH : LED_LOW;
}

/* 在注册表中查找指定编号的 LED 配置。 */
static const LED_Config_t *LED_FindConfig(LED_Id_t id)
{
	uint8_t index;

	if ((s_ledConfigTable == 0) || (s_ledConfigCount == 0U))
	{
		return 0;
	}

	for (index = 0U; index < s_ledConfigCount; ++index)
	{
		if (s_ledConfigTable[index].id == id)
		{
			return &s_ledConfigTable[index];
		}
	}

	return 0;
}

/*
 * LED_Register：把 Enroll 层提供的 LED 资源表存起来。
 * 这里不直接访问硬件，只负责保存“哪个 LED 对应哪个 port/pin”。
 */
void LED_Register(const LED_Config_t *configTable, uint8_t count)
{
	/* 先清空注册状态，避免旧表被继续使用。 */
	s_ledConfigTable = 0;
	s_ledConfigCount = 0U;

	if ((configTable == 0) || (count == 0U))
	{
		return;
	}

	s_ledConfigTable = configTable;
	s_ledConfigCount = count;

	/* 注册后重置闪烁上下文，避免切换配置表时继承旧状态。 */
	(void)memset(s_ledTurnCtx, 0, sizeof(s_ledTurnCtx));

	/* 注册阶段只保存表，不做 IO 电平写入。 */
}

/*
 * LED_Init：统一初始化所有已注册 LED。
 * initLevel 的意义是“上电后默认把所有 LED 拉成什么电平”，
 * 这样可以避免 MCU 复位后因为默认态不确定而误亮。
 */
void LED_Init(LED_Level_t initLevel)
{
	/* index: 遍历注册表索引。 */
	uint8_t index;
	/* config: 当前正在处理的 LED 配置项。 */
	const LED_Config_t *config;
	/* normalizedInitLevel: 归一化后的初始化电平。 */
	LED_Level_t normalizedInitLevel;

	normalizedInitLevel = LED_NormalizeLevel(initLevel);

	if ((s_ledConfigTable == 0) || (s_ledConfigCount == 0U))
	{
		return;
	}

	for (index = 0U; index < s_ledConfigCount; ++index)
	{
		config = &s_ledConfigTable[index];
		if ((config->gpioInit == 0) || (config->gpioWrite == 0) || (config->port == 0) || (config->pin == 0U))
		{
			continue;
		}

		/* 按传入的 LED_HIGH / LED_LOW 统一设置初始化电平。 */
		config->gpioInit(config->port, config->pin);
		config->gpioWrite(config->port, config->pin, normalizedInitLevel);
	}
}

/* LED_Control：控制指定编号的 LED 输出高/低电平。 */
void LED_Control(LED_Id_t id, LED_Level_t level)
{
	/* config: 查找到的 LED 映射配置。 */
	const LED_Config_t *config;
	LED_Level_t normalized;

	config = LED_FindConfig(id);
	if (config == 0)
	{
		return;
	}

	/* 电平控制 */
	normalized = LED_NormalizeLevel(level);
	config->gpioWrite(config->port, config->pin, normalized);

	/* 同步保存当前电平，供 LED_Turn 做非阻塞翻转。 */
	if ((uint32_t)id < (uint32_t)(sizeof(s_ledTurnCtx) / sizeof(s_ledTurnCtx[0])))
	{
		s_ledTurnCtx[id].currentLevel = normalized;
		s_ledTurnCtx[id].levelKnown = 1U;
	}
}

/*
 * LED_Turn：阻塞翻转接口。
 * 每次调用执行一次翻转，然后阻塞 periodMs。
 */
void LED_Turn(LED_Id_t id, uint16_t periodMs)
{
	if ((periodMs == 0U) || ((uint32_t)id >= (uint32_t)(sizeof(s_ledTurnCtx) / sizeof(s_ledTurnCtx[0]))))
	{
		return;
	}

	if (s_ledTurnCtx[id].levelKnown == 0U)
	{
		s_ledTurnCtx[id].currentLevel = LED_LOW;
		s_ledTurnCtx[id].levelKnown = 1U;
	}

	s_ledTurnCtx[id].currentLevel =
		(s_ledTurnCtx[id].currentLevel == LED_LOW) ? LED_HIGH : LED_LOW;
	LED_Control(id, s_ledTurnCtx[id].currentLevel);
	vTaskDelay(pdMS_TO_TICKS((uint32_t)periodMs));
}

#include "KEY.h"

/* 已注册的按键配置表。 */
static const KEY_Config_t *s_keyConfigTable;
/* 当前配置表中的按键数量。 */
static uint8_t s_keyConfigCount;

/* Key_Num：按键扫描完成后暂存的一次有效键值。 */
uint8_t Key_Num;
/* Key：对外暴露的按键键值，沿用原工程的全局变量用法。 */
uint8_t Key = 0; /* 按键键值 */

/*
 * 从注册表里取当前使用的按键配置。
 * 当前工程按单按键使用，直接返回第 1 项配置。
 */
static const KEY_Config_t *KEY_GetConfig(void)
{
	if ((s_keyConfigTable == 0) || (s_keyConfigCount == 0U))
	{
		return 0;
	}

	return &s_keyConfigTable[0];
}

/*
 * KEY_Register：把 Enroll 层提供的按键资源表保存下来。
 * 这里只登记映射，不直接碰硬件。
 */
void KEY_Register(const KEY_Config_t *configTable, uint8_t count)
{
	s_keyConfigTable = 0;
	s_keyConfigCount = 0U;

	if ((configTable == 0) || (count == 0U))
	{
		return;
	}

	s_keyConfigTable = configTable;
	s_keyConfigCount = count;
}

/*
 * KEY_Init：初始化已注册的按键 GPIO。
 * 具体输入模式由注册表里的 gpioInit 函数决定。
 */
void KEY_Init(void)
{
	const KEY_Config_t *config;

	config = KEY_GetConfig();
	if (config == 0)
	{
		return;
	}

	if (config->gpioInit != 0)
	{
		config->gpioInit(config->port, config->pin);
	}
}

/*
 * Key_GetNum：读取一次按键结果。
 * 返回非 0 表示有新的按键事件，读取后会清空缓冲值。
 */
uint8_t Key_GetNum(void)
{
	uint8_t Temp;
	if (Key_Num)
	{
		Temp = Key_Num;
		Key_Num = 0;
		return Temp;
	}
	return 0;
}

/*
 * Key_GetState：读取当前按键是否按下。
 * 低电平按下，返回 1 表示KEY1按下。
 */
uint8_t Key_GetState(void)
{
	const KEY_Config_t *config;

	config = KEY_GetConfig();
	if ((config == 0) || (config->gpioRead == 0))
	{
		return 0U;
	}

	if (config->gpioRead(config->port, config->pin) == 0U)
	{
		return 1; /* KEY1按下 */
	}
	return 0;
}

/*
 * Key_Tick：按键扫描函数。
 */
void Key_Tick(void)
{
	static uint8_t Count; /* 按键计数器 */
	static uint8_t CurrState, PrevState; /* 当前状态和前一状态 */

	Count ++;
	if (Count >= 20)
	{
		Count = 0;

		PrevState = CurrState;
		CurrState = Key_GetState();

		if (CurrState == 0 && PrevState != 0)
		{
			Key_Num = PrevState;
		}
	}
}

/*
 * key_Get：把扫描得到的按键值同步到全局 Key。
 */
void key_Get(void)
{
	static uint8_t KeyNum = 0;

	KeyNum = Key_GetNum();
	if(KeyNum)
	{
		Key = KeyNum;
		KeyNum = 0;
	}
}



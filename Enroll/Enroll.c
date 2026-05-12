#include "Enroll.h"

/*
 * Enroll 注册层：
 * 1) 读取 hw_config.h 中的板级映射
 * 2) 对上层只暴露初始化和控制入口
 */

/*
 * ENROLL_LED_ITEM 负责把宏映射展开成 LED_Config_t 结构体项。
 */
#define ENROLL_LED_ITEM(id, port, pin) \
	{ id, port, pin, ENROLL_GPIO_INIT_FN, ENROLL_GPIO_WRITE_FN },


/* 当前板子的 LED 注册表。 */
static const LED_Config_t s_ledTable[] =
{
	HW_LED_MAP(ENROLL_LED_ITEM)
};

#undef ENROLL_LED_ITEM

/* ENROLL_USART_ITEM 负责把板级 USART 宏映射展开成 API 配置项。 */
#define ENROLL_USART_ITEM(id, coreId, txPort, txPin, rxPort, rxPin) \
	{ id, coreId, txPort, txPin, rxPort, rxPin },

static const API_USART_Config_t s_usartTable[] =
{
	HW_USART_MAP(ENROLL_USART_ITEM)
};

#undef ENROLL_USART_ITEM

/* ENROLL_PWM_ITEM 负责把板级 PWM 宏映射展开成 API 配置项。 */
#define ENROLL_PWM_ITEM(timId, channel, coreTimId, coreChannel, port, pin) \
	{ timId, channel, coreTimId, coreChannel, port, pin },

static const API_PWM_Config_t s_pwmTable[] =
{
	HW_PWM_MAP(ENROLL_PWM_ITEM)
};

#undef ENROLL_PWM_ITEM

/* ENROLL_TIM_ITEM 负责把板级 TIM 宏映射展开成 API 配置项。 */
#define ENROLL_TIM_ITEM(id, coreId) \
	{ id, coreId },

static const API_TIM_Config_t s_timTable[] =
{
	HW_TIM_MAP(ENROLL_TIM_ITEM)
};

#undef ENROLL_TIM_ITEM

/*
 * Enroll_LED_Init：
 * 先把板级 LED 映射注册到 BSP，再按指定电平完成初始化。
 */
void Enroll_LED_Init(LED_Level_t initLevel)
{
	LED_Register(s_ledTable, HW_LED_COUNT);
	LED_Init(initLevel);
}

/* Enroll_LED_Control：把控制请求转发给 BSP。 */
void Enroll_LED_Control(LED_Id_t id, LED_Level_t level)
{
	LED_Control(id, level);
}

/* Enroll_USART_Init：注册板级 USART 后初始化指定串口。 */
void Enroll_USART_Init(API_USART_Id_t id, uint32_t baudRate)
{
	API_USART_Register(s_usartTable, HW_USART_COUNT);
	API_USART_Init(id, baudRate);
}

void Enroll_USART_RegisterIrqHandler(API_USART_IrqHandler_t handler)
{
	uint8_t i;

	API_USART_Register(s_usartTable, HW_USART_COUNT);
	for (i = 0U; i < HW_USART_COUNT; ++i)
	{
		API_USART_RegisterIrqHandler(s_usartTable[i].id, handler);
	}
}

void Enroll_PWM_Init(API_PWM_Tim_t timId, uint16_t arr, uint16_t psc)
{
	API_PWM_Register(s_pwmTable, HW_PWM_COUNT);
	API_PWM_Init(timId, arr, psc);
}

void Enroll_TIM_RegisterIrqHandler(API_TIM_IrqHandler_t handler)
{
	uint8_t i;

	API_TIM_Register(s_timTable, HW_TIM_COUNT);
	for (i = 0U; i < HW_TIM_COUNT; ++i)
	{
		API_TIM_RegisterIrqHandler(s_timTable[i].id, handler);
	}
}

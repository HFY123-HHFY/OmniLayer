#include "Enroll.h"

/*
 * Enroll 注册层：
 * 1) 读取 hw_config.h 中的板级映射
 * 2) 把 BSP 抽象 LED 和当前目标 MCU 的 GPIO 驱动连起来
 * 3) 对上层只暴露初始化和控制入口
 */

/*
 * ENROLL_LED_ITEM 负责把宏映射展开成 LED_Config_t 结构体项。
 * 这样硬件列表统一维护在 103_hw_config.h，Enroll.c 只保留一份模板。
 */
#define ENROLL_LED_ITEM(id, port, pin) \
	{ id, port, pin, ENROLL_GPIO_INIT_FN, ENROLL_GPIO_WRITE_FN },

/* 当前板子的 LED 注册表。 */
static const LED_Config_t s_ledTable[] =
{
	HW_LED_MAP(ENROLL_LED_ITEM)
};

#undef ENROLL_LED_ITEM

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

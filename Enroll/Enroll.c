#include "Enroll.h"
#include "MPU6050_Int.h"
#include "pwm.h"

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

/* ENROLL_USART_ITEM 负责把串口映射展开成 API_USART_Config_t 结构体项。 */
#define ENROLL_USART_ITEM(id, txPort, txPin, rxPort, rxPin) \
	{ id, txPort, txPin, rxPort, rxPin },

/* ENROLL_I2C_ITEM 负责把 I2C 映射展开成 MyI2C_Config_t 结构体项。 */
#define ENROLL_I2C_ITEM(port, sclPin, sdaPin) \
	{ port, sclPin, sdaPin },

/* ENROLL_KEY_ITEM 负责把按键映射展开成 KEY_Config_t 结构体项。 */
#define ENROLL_KEY_ITEM(id, port, pin) \
	{ id, port, pin, ENROLL_GPIO_INPUT_FN, ENROLL_GPIO_READ_FN },

/* ENROLL_PWM_ITEM 负责把 PWM 映射展开成 API_PWM_Config_t 结构体项。 */
#define ENROLL_PWM_ITEM(timId, channel, port, pin) \
	{ timId, channel, port, pin },

/* 当前板子的 LED 注册表。 */
static const LED_Config_t s_ledTable[] =
{
	HW_LED_MAP(ENROLL_LED_ITEM)
};

/* 当前板子的 USART 注册表。 */
static const API_USART_Config_t s_usartTable[] =
{
	HW_USART_MAP(ENROLL_USART_ITEM)
};

/* 当前板子的 I2C 注册表。 */
static const MyI2C_Config_t s_i2cTable[] =
{
	HW_I2C_MAP(ENROLL_I2C_ITEM)
};

/* 当前板子的 KEY 注册表。 */
static const KEY_Config_t s_keyTable[] =
{
	HW_KEY_MAP(ENROLL_KEY_ITEM)
};

/* 当前板子的 PWM 注册表。 */
static const API_PWM_Config_t s_pwmTable[] =
{
	HW_PWM_MAP(ENROLL_PWM_ITEM)
};

#undef ENROLL_LED_ITEM
#undef ENROLL_USART_ITEM
#undef ENROLL_I2C_ITEM
#undef ENROLL_KEY_ITEM
#undef ENROLL_PWM_ITEM

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

/* Enroll_USART_Register：把当前板子的 USART 引脚映射注册给 API 层。 */
void Enroll_USART_Register(void)
{
	API_USART_Register(s_usartTable, HW_USART_COUNT);
}

/* Enroll_I2C_Register：把当前板子的 I2C 引脚映射注册给应用层。 */
void Enroll_I2C_Register(void)
{
	MyI2C_Register(s_i2cTable, HW_I2C_COUNT);
}

/* Enroll_PWM_Register：把当前板子的 PWM 引脚映射注册给 API 层。 */
void Enroll_PWM_Register(void)
{
	API_PWM_Register(s_pwmTable, HW_PWM_COUNT);
}

/* Enroll_MPU6050_EXTI_Register：按板级配置注册 MPU6050 INT 外部中断。 */
void Enroll_MPU6050_EXTI_Register(void)
{
	/* 板级配置仅提供引脚映射，默认触发沿/优先级由 MPU6050_Int.c 定义。 */
	MPU6050_EXTI_InitBoard((void *)HW_MPU6050_INT_PORT, HW_MPU6050_INT_PIN);
}

/* Enroll_KEY_Init：把当前板子的按键映射注册给 BSP 并完成初始化。 */
void Enroll_KEY_Init(void)
{
	KEY_Register(s_keyTable, HW_KEY_COUNT);
	KEY_Init();
}

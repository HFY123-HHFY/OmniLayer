#include "Enroll.h"

#include "OLED.h"         /* OLED_SpiCtrlConfig_t, OLED_RegisterSpiCtrl */
#include "exti.h"         /* API_EXTI_Config_t, API_EXTI_xxx */
#include "MPU6050_Int.h"  /* MPU6050_EXTI_Callback */
#include <stddef.h>

/*
 * Enroll.c 仅放“注册实现”与“门面转发”：
 * - 通过 HW_xxx_MAP 宏展开各外设配置表；
 * - 调用 API/BSP 的 Register/Init 函数完成绑定；
 * - 不直接实现外设控制逻辑。
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

/* ENROLL_KEY_ITEM 负责把板级 KEY 宏映射展开成 BSP 配置项。 */
#define ENROLL_KEY_ITEM(id, port, pin) \
	{ id, port, pin, ENROLL_GPIO_INPUT_FN, ENROLL_GPIO_READ_FN },

static const KEY_Config_t s_keyTable[] =
{
	HW_KEY_MAP(ENROLL_KEY_ITEM)
};

#undef ENROLL_KEY_ITEM

/* ENROLL_I2C_ITEM 负责把板级 I2C 宏映射展开成软件 I2C 配置项。 */
#define ENROLL_I2C_ITEM(id, port, sclPin, sdaPin) \
	{ id, port, sclPin, sdaPin },

static const MyI2C_Config_t s_i2cTable[] =
{
	HW_I2C_MAP(ENROLL_I2C_ITEM)
};

#undef ENROLL_I2C_ITEM

/* ENROLL_SPI_ITEM 负责把板级 SPI 宏映射展开成软件 SPI 配置项。 */
#define ENROLL_SPI_ITEM(id, csPort, csPin, sckPort, sckPin, mosiPort, mosiPin, misoPort, misoPin) \
	{ id, csPort, csPin, sckPort, sckPin, mosiPort, mosiPin, misoPort, misoPin },

static const MySPI_Config_t s_spiTable[] =
{
	HW_SPI_MAP(ENROLL_SPI_ITEM)
};

#undef ENROLL_SPI_ITEM

/* 注册软件 SPI 资源表。 */
void Enroll_SPI_Register(void)
{
	MySPI_Register(s_spiTable, HW_SPI_COUNT);
}

/* ENROLL_OLED_SPI_CTRL_ITEM 负责展开 OLED 的 DC/RES 控制引脚映射。 */
#define ENROLL_OLED_SPI_CTRL_ITEM(dcPort, dcPin, resPort, resPin) \
	{ dcPort, dcPin, resPort, resPin },

static const OLED_SpiCtrlConfig_t s_oledSpiCtrlTable[] =
{
	HW_OLED_SPI_CTRL_MAP(ENROLL_OLED_SPI_CTRL_ITEM)
};

#undef ENROLL_OLED_SPI_CTRL_ITEM

/* 注册 OLED 在 SPI 模式下使用的控制引脚映射。 */
void Enroll_OLED_Register(void)
{
	OLED_RegisterSpiCtrl(s_oledSpiCtrlTable, HW_OLED_SPI_CTRL_COUNT);
}

/* MPU6050 INT 使用的 EXTI 注册表。 */
static const API_EXTI_Config_t s_mpuExtiTable[] =
{
	{ 0U, HW_MPU6050_INT_PORT, HW_MPU6050_INT_PIN }
};

/*
 * 注册 MPU6050 EXTI：
 * 1) 注册 id->port/pin 映射
 * 2) 绑定 MPU6050 的中断回调
 * 3) 配置触发沿与中断优先级
 */
void Enroll_MPU6050_Register(void)
{
	API_EXTI_Register(s_mpuExtiTable, 1U);
	/* 多路注册示例：同一 id 可继续追加其他回调。 */
	API_EXTI_AddIrqHandler(s_mpuExtiTable[0].id, (API_EXTI_IrqHandler_t)MPU6050_EXTI_Callback, NULL);
	/* API_EXTI_AddIrqHandler(s_mpuExtiTable[0].id, Other_EXTI_Callback, userPtr); */
	API_EXTI_Init(s_mpuExtiTable[0].id, API_EXTI_TRIGGER_RISING, 0U, 2U);
}

/* ENROLL_PWM_ITEM 负责把板级 PWM 宏映射展开成 API 配置项。 */
#define ENROLL_PWM_ITEM(timId, channel, coreTimId, coreChannel, port, pin) \
	{ timId, channel, coreTimId, coreChannel, port, pin },

static const API_PWM_Config_t s_pwmTable[] =
{
	HW_PWM_MAP(ENROLL_PWM_ITEM)
};

#undef ENROLL_PWM_ITEM

/* ENROLL_ADC_ITEM 负责把板级 ADC 宏映射展开成 API 配置项。 */
#define ENROLL_ADC_ITEM(id, channel, port, pin) \
	{ id, channel, port, pin },

static const API_ADC_Config_t s_adcTable[] =
{
	HW_ADC_MAP(ENROLL_ADC_ITEM)
};

#undef ENROLL_ADC_ITEM

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

void Enroll_KEY_Init(void)
{
	KEY_Register(s_keyTable, HW_KEY_COUNT);
	KEY_Init();
}

/* 仅注册 I2C 资源，不在此处做总线初始化时序。 */
void Enroll_I2C_Register(void)
{
	MyI2C_Register(s_i2cTable, HW_I2C_COUNT);
}

/* 注册 PWM 资源并初始化指定逻辑定时器。 */
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

void Enroll_ADC_Init(API_ADC_Id_t id)
{
	API_ADC_Register(s_adcTable, HW_ADC_COUNT);
	API_ADC_Init(id);
}

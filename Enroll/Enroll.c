#include "Enroll_Internal.h"
#include "IrqPriority.h"

/*
 * Enroll.c 职责：
 * 1) 把板级映射展开成各外设配置表；
 * 2) 调用 API/BSP 的 Register/Init 完成资源登记；
 * 3) 仅做注册与门面转发，不实现具体外设控制逻辑。
 */

/*************************** API配置层 ********************************/
/*******************************PWM***********************************/
/* PWM 配置表：把 HW_PWM_MAP 展开成 API_PWM_Config_t。 */
#define ENROLL_PWM_ITEM(timId, channel, coreTimId, coreChannel, port, pin) \
	{ timId, channel, coreTimId, coreChannel, port, pin },

static const API_PWM_Config_t s_pwmTable[] =
{
	HW_PWM_MAP(ENROLL_PWM_ITEM)
};
#undef ENROLL_PWM_ITEM

/*******************************ADC***********************************/
/* ADC 配置表：把 HW_ADC_MAP 展开成 API_ADC_Config_t。 */
#define ENROLL_ADC_ITEM(id, channel, port, pin) \
	{ id, channel, port, pin },

static const API_ADC_Config_t s_adcTable[] =
{
	HW_ADC_MAP(ENROLL_ADC_ITEM)
};
#undef ENROLL_ADC_ITEM

/*******************************TIM***********************************/
/* TIM 配置表：把 HW_TIM_MAP 展开成 API_TIM_Config_t。 */
#define ENROLL_TIM_ITEM(id, coreId) \
	{ id, coreId },

static const API_TIM_Config_t s_timTable[] =
{
	HW_TIM_MAP(ENROLL_TIM_ITEM)
};
#undef ENROLL_TIM_ITEM

/*******************************USART***********************************/
/* USART 配置表：把 HW_USART_MAP 展开成 API_USART_Config_t。 */
#define ENROLL_USART_ITEM(id, coreId, txPort, txPin, rxPort, rxPin) \
	{ id, coreId, txPort, txPin, rxPort, rxPin },

static const API_USART_Config_t s_usartTable[] =
{
	HW_USART_MAP(ENROLL_USART_ITEM)
};
#undef ENROLL_USART_ITEM

/* ENCODER 编码器配置表：把 HW_ENCODER_MAP 展开成 API_Encoder_Config_t。 */
#define ENROLL_ENCODER_ITEM(id, coreId, chA, portA, pinA, chB, portB, pinB) \
	{ id, coreId, chA, portA, pinA, chB, portB, pinB },

static const API_Encoder_Config_t s_encoderTable[] =
{
	HW_ENCODER_MAP(ENROLL_ENCODER_ITEM)
};
#undef ENROLL_ENCODER_ITEM

/*************************** I2C/SPI协议配置层 ************************/
/*******************************I2C***********************************/
/* I2C 配置表：把 HW_I2C_MAP 展开成 API_I2C_Config_t。 */
#define ENROLL_I2C_ITEM(id, sclPort, sclPin, sdaPort, sdaPin, sclIomux, sdaIomux) \
	{ id, sclPort, sclPin, sclIomux, sdaPort, sdaPin, sdaIomux },

static const API_I2C_Config_t s_i2cTable[] =
{
	HW_I2C_MAP(ENROLL_I2C_ITEM)
};
#undef ENROLL_I2C_ITEM

/*******************************SPI***********************************/
/* SPI 配置表：把 HW_SPI_MAP 展开成 API_SPI_Config_t。 */
#define ENROLL_SPI_ITEM(id, csPort, csPin, sckPort, sckPin, mosiPort, mosiPin, misoPort, misoPin, csIomux, sckIomux, mosiIomux, misoIomux) \
	{ id, csPort, csPin, csIomux, sckPort, sckPin, sckIomux, mosiPort, mosiPin, mosiIomux, misoPort, misoPin, misoIomux },

static const API_SPI_Config_t s_spiTable[] =
{
	HW_SPI_MAP(ENROLL_SPI_ITEM)
};
#undef ENROLL_SPI_ITEM

/****************************** BSP配置层 *****************************/
/*******************************LED***********************************/
/* LED 配置表：把 HW_LED_MAP 展开成 LED_Config_t。 */
#define ENROLL_LED_ITEM(id, port, pin) \
	{ id, port, pin, ENROLL_GPIO_INIT_FN, ENROLL_GPIO_WRITE_FN },

/* 当前板子的 LED 注册表。 */
static const LED_Config_t s_ledTable[] =
{
	HW_LED_MAP(ENROLL_LED_ITEM)
};
#undef ENROLL_LED_ITEM

/*******************************KEY***********************************/
/* KEY 配置表：把 HW_KEY_MAP 展开成 KEY_Config_t。 */
#define ENROLL_KEY_ITEM(id, port, pin) \
	{ id, port, pin, ENROLL_GPIO_INPUT_FN, ENROLL_GPIO_READ_FN },

static const KEY_Config_t s_keyTable[] =
{
	HW_KEY_MAP(ENROLL_KEY_ITEM)
};
#undef ENROLL_KEY_ITEM

/* OLED SPI 控制表：把 HW_OLED_SPI_CTRL_MAP 展开成 OLED_SpiCtrlConfig_t。 */
#define ENROLL_OLED_SPI_CTRL_ITEM(dcPort, dcPin, resPort, resPin) \
	{ dcPort, dcPin, resPort, resPin },

static const OLED_SpiCtrlConfig_t s_oledSpiCtrlTable[] =
{
	HW_OLED_SPI_CTRL_MAP(ENROLL_OLED_SPI_CTRL_ITEM)
};
#undef ENROLL_OLED_SPI_CTRL_ITEM

/* MPU6050 EXTI 表：登记外部中断输入引脚。 */
static const API_EXTI_Config_t s_mpuExtiTable[] =
{
	{ 0U, HW_MPU6050_INT_PORT, HW_MPU6050_INT_PIN }
};

/* TB6612 配置表：把 HW_TB6612_MAP 展开成 TB6612_Config_t。 */
#define ENROLL_TB6612_ITEM(ain1Port, ain1Pin, ain2Port, ain2Pin, bin1Port, bin1Pin, bin2Port, bin2Pin) \
	{ ain1Port, ain1Pin, ain2Port, ain2Pin, bin1Port, bin1Pin, bin2Port, bin2Pin },

static const TB6612_Config_t s_tb6612Table[] =
{
	HW_TB6612_MAP(ENROLL_TB6612_ITEM)
};
#undef ENROLL_TB6612_ITEM

/****************************** API资源注册层 ************************/
/* PWM 注册：登记板级 PWM 资源表。 */
void Enroll_PWM_Register(void)
{
	API_PWM_Register(s_pwmTable, HW_PWM_COUNT);
}

/* ADC 注册：登记板级 ADC 资源表。 */
void Enroll_ADC_Register(void)
{
	API_ADC_Register(s_adcTable, HW_ADC_COUNT);
}

/* TIM 注册：登记板级 TIM 资源表。 */
void Enroll_TIM_Register(void)
{
	API_TIM_Register(s_timTable, HW_TIM_COUNT);
}

/* TIM 中断回调注册：遍历所有板级定时器。 */
void Enroll_TIM_RegisterIrqHandler(API_TIM_IrqHandler_t handler)
{
	uint8_t i;

	for (i = 0U; i < HW_TIM_COUNT; ++i)
	{
		API_TIM_RegisterIrqHandler(s_timTable[i].id, handler);
	}
}

/* USART 注册：登记板级 USART 资源表。 */
void Enroll_USART_Register(void)
{
	API_USART_Register(s_usartTable, HW_USART_COUNT);
}

/* USART 中断回调注册：遍历所有板级 USART。 */
void Enroll_USART_RegisterIrqHandler(API_USART_IrqHandler_t handler)
{
	uint8_t i;

	for (i = 0U; i < HW_USART_COUNT; ++i)
	{
		API_USART_RegisterIrqHandler(s_usartTable[i].id, handler);
	}
}

/***************** I2C/SPI协议资源注册层 *********************/
/* I2C 注册：仅登记资源表，不在这里做总线初始化。 */
void Enroll_I2C_Register(void)
{
	API_I2C_Register(s_i2cTable, HW_I2C_COUNT);
}

/* SPI 注册：把板级 SPI 资源表登记到 API_SPI 模块。 */
void Enroll_SPI_Register(void)
{
	API_SPI_Register(s_spiTable, HW_SPI_COUNT);
}

/***************** BSP层资源注册层 *********************/
/* LED 注册：登记板级 LED 资源表。 */
void Enroll_LED_Register(void)
{
	LED_Register(s_ledTable, HW_LED_COUNT);
}

/* KEY 注册：登记板级 KEY 资源表。 */
void Enroll_KEY_Register(void)
{
	KEY_Register(s_keyTable, HW_KEY_COUNT);
}

/* OLED 注册：登记 SPI 模式下的 DC/RES 控制引脚。 */
void Enroll_OLED_Register(void)
{
	OLED_RegisterSpiCtrl(s_oledSpiCtrlTable, HW_OLED_SPI_CTRL_COUNT);
}

/*
 * MPU6050 注册：
 * 1) 登记 EXTI 资源表；
 * 2) 绑定 MPU6050 中断回调；
 * 3) 配置触发沿与优先级。
 */
void Enroll_MPU6050_Register(void)
{
	API_EXTI_Register(s_mpuExtiTable, 1U);
	/* 同一 id 可继续追加其他回调。 */
	API_EXTI_AddIrqHandler(s_mpuExtiTable[0].id, (API_EXTI_IrqHandler_t)MPU6050_EXTI_Callback, NULL);
	/* API_EXTI_AddIrqHandler(s_mpuExtiTable[0].id, Other_EXTI_Callback, userPtr); */
	API_EXTI_Init(s_mpuExtiTable[0].id, API_EXTI_TRIGGER_RISING, IRQ_PRIO_MPU6050, IRQ_SUB_PRIO_MPU6050);
}

/* TB6612 注册 */
void Enroll_TB6612_Register(void)
{
	TB6612_Register(s_tb6612Table, HW_TB6612_COUNT);
}

/* 编码器注册 */
void Enroll_Encoder_Register(void)
{
	API_Encoder_Register(s_encoderTable, HW_ENCODER_COUNT);
}

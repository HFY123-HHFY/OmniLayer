#include "TB6612.h"
#include "pwm.h"
#include "gpio.h"

/* TB6612 配置表指针，由 Enroll 阶段注册。 */
static const TB6612_Config_t *s_tb6612ConfigTable;
/* TB6612 配置项数量。 */
static uint8_t s_tb6612ConfigCount;

/* 获取当前生效的 TB6612 配置，未注册时返回 0。 */
static const TB6612_Config_t *TB6612_GetConfig(void)
{
	if ((s_tb6612ConfigTable == 0) || (s_tb6612ConfigCount == 0U))
	{
		return 0;
	}
	return &s_tb6612ConfigTable[0];
}

/*
 * 将输入速度绝对值转换为占空比，并限制在 0~TB6612_MAX_DUTY。
 */
static uint16_t TB6612_AbsToDuty(int16_t value)
{
	uint32_t duty;
	const TB6612_Config_t *config;

	config = TB6612_GetConfig();
	if (config == 0)
	{
		return 0U;
	}

	if (value < 0)
	{
		duty = (uint32_t)(-value);
	}
	else
	{
		duty = (uint32_t)value;
	}

	if (duty > TB6612_MAX_DUTY)
	{
		duty = TB6612_MAX_DUTY;
	}
	return (uint16_t)duty;
}

/*
 * 初始化单个方向引脚为推挽输出、无上下拉。
 * 通过统一 API_GPIO_InitOutput 由各芯片后端落实具体寄存器配置。
 */
static void TB6612_InitDirPin(void *port, uint32_t pin)
{
	API_GPIO_InitOutput(port, pin);
}

void TB6612_Register(const TB6612_Config_t *configTable, uint8_t count)
{
	s_tb6612ConfigTable = configTable;
	s_tb6612ConfigCount = count;
}

/* 初始化 TB6612 方向脚并默认全部拉低，避免上电误动作。 */
void TB6612_Init(void)
{
	const TB6612_Config_t *config;

	config = TB6612_GetConfig();
	if (config == 0)
	{
		return;
	}

	TB6612_InitDirPin(config->ain1Port, config->ain1Pin);
	TB6612_InitDirPin(config->ain2Port, config->ain2Pin);
	TB6612_InitDirPin(config->bin1Port, config->bin1Pin);
	TB6612_InitDirPin(config->bin2Port, config->bin2Pin);

	AIN1_OUT(0);
	AIN2_OUT(0);
	BIN1_OUT(0);
	BIN2_OUT(0);
}

/* 根据速度符号设置方向脚，并通过 PWM 输出对应占空比。 */
void TB6612_SetSpeed(int16_t speedA, int16_t speedB)
{
	const TB6612_Config_t *config;
	uint16_t dutyA = 0U, dutyB = 0U;

	config = TB6612_GetConfig();
	if (config == 0)
	{
		return;
	}

	dutyA = TB6612_AbsToDuty(speedA);
	dutyB = TB6612_AbsToDuty(speedB);

	// --------------------- A电机 ---------------------
	if (speedA > 0)
	{
		// 正转
		AIN1_OUT(0);
		AIN2_OUT(1);
	}
	else if (speedA < 0)
	{
		// 反转
		AIN1_OUT(1);
		AIN2_OUT(0);
	}
	else
	{
		// 停止 = 方向脚全部置0
		AIN1_OUT(0);
		AIN2_OUT(0);
	}

	// 设置占空比
	API_PWM_Setcom(TB6612_PWM_TIM, TB6612_PWM_CH_A, dutyA);

	// --------------------- B电机 ---------------------
	if (speedB > 0)
	{
		// 正转
		BIN1_OUT(1);
		BIN2_OUT(0);
	}
	else if (speedB < 0)
	{
		// 反转
		BIN1_OUT(0);
		BIN2_OUT(1);
	}
	else
	{
		// 停止 = 方向脚全部置0
		BIN1_OUT(0);
		BIN2_OUT(0);
	}

	// 设置占空比
	API_PWM_Setcom(TB6612_PWM_TIM, TB6612_PWM_CH_B, dutyB);
}

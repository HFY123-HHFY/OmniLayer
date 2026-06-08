#ifndef __TB6612_H
#define __TB6612_H

#include <stdint.h>

/* TB6612 默认使用的 PWM 定时器与通道映射。 */
#define TB6612_PWM_TIM        (API_PWM_TIM1)
#define TB6612_PWM_CH_A       (API_PWM_CH2)
#define TB6612_PWM_CH_B       (API_PWM_CH1)

/* TB6612 占空比上限*/
#define TB6612_MAX_DUTY       (400U) /* 占空比400-编码器 - 90左右, 占空比200-编码器 - 42左右 */

#define TB6612_WRITE(port, pin, level) API_GPIO_Write((port), (pin), (uint8_t)((level) ? 1U : 0U))

/* 方向控制快捷宏：AIN1/AIN2/BIN1/BIN2。 */
#define AIN1_OUT(x) TB6612_WRITE(s_tb6612ConfigTable[0].ain1Port, s_tb6612ConfigTable[0].ain1Pin, (x))
#define AIN2_OUT(x) TB6612_WRITE(s_tb6612ConfigTable[0].ain2Port, s_tb6612ConfigTable[0].ain2Pin, (x))
#define BIN1_OUT(x) TB6612_WRITE(s_tb6612ConfigTable[0].bin1Port, s_tb6612ConfigTable[0].bin1Pin, (x))
#define BIN2_OUT(x) TB6612_WRITE(s_tb6612ConfigTable[0].bin2Port, s_tb6612ConfigTable[0].bin2Pin, (x))

/* TB6612 方向脚资源映射，仅包含 AIN/BIN 四个方向控制脚。 */
typedef struct
{
	void *ain1Port;
	uint32_t ain1Pin;
	void *ain2Port;
	uint32_t ain2Pin;
	void *bin1Port;
	uint32_t bin1Pin;
	void *bin2Port;
	uint32_t bin2Pin;
} TB6612_Config_t;

/* 注册 TB6612 配置表。 */
void TB6612_Register(const TB6612_Config_t *configTable, uint8_t count);
/* 初始化 TB6612 方向脚。 */
void TB6612_Init(void);
/* 设置 A/B 两路电机速度（正负表示方向，绝对值表示占空比）。 */
void TB6612_SetSpeed(int16_t speedA, int16_t speedB);

#endif /* __TB6612_H__ */

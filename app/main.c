/* Enroll 注册层，负责把板级资源注册到 BSP */
#include "Enroll.h"

/*系统sys层*/
#include "Delay.h"

/*API层 MCU片内外设*/
#include "usart.h"
#include "tim.h"
#include "pwm.h"

/*app应用层*/
#include "My_Usart/My_Usart.h"
#include "Control_Task/Control_Task.h"

/*BSP硬件抽象层*/
#include "LED.h"

int main(void)
{
/* 板子注册层初始化 */
	Enroll_LED_Init(LED_LOW); 				/* LED 资源注册 */
	Enroll_USART_Init(API_USART1, 115200U);	/* USART 资源注册 */
	Enroll_USART_RegisterIrqHandler(Control_Task_USART_Callback); 	/* 串口中断回调注册 */
	Enroll_TIM_RegisterIrqHandler(Control_Task_TIM_Callback); 		/* 定时器中断回调注册 */

	// Enroll_PWM_Init(API_PWM_TIM1, 400U - 1U, 8U - 1U); /* G3507 PWM: TIM1 -> 10kHz */
	// Enroll_PWM_Init(API_PWM_TIM2, 100U - 1U, 720U - 1U); /* 103 PWM: TIM2 -> 1kHz */
	// Enroll_PWM_Init(API_PWM_TIM1, 4000U - 1U, 840U - 1U); /* 407 PWM: TIM1 -> 50Hz */

	API_TIM_Init(API_TIM1, 1U); /* 定时器初始化：API_TIM1，每 1ms 触发一次更新中断 */

	while (1)
	{
/* LED 测试 */
		// LED_Control(LED1, LED_HIGH);

/*串口测试*/
	usart_printf(USART1, "Timer_Bsp_t: %lu\r\n", Timer_Bsp_t);
	Delay_ms(500U);
	Delay_ms(500U);

/*PWM测试*/
		// API_PWM_Setcom(API_PWM_TIM1, API_PWM_CH1, 100U);
		// API_PWM_Setcom(API_PWM_TIM1, API_PWM_CH2, 300U);
	}
}

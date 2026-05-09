/* Enroll 注册层，负责把板级资源注册到 BSP */
#include "Enroll.h"

/*系统sys层*/
#include "Delay.h"

/*API层 MCU片内外设*/
#include "usart.h"
#include "tim.h"

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
	Enroll_USART_RegisterIrqHandler(Control_Task_USART_Callback); /* 串口中断回调注册 */
	Enroll_TIM_RegisterIrqHandler(Control_Task_TIM_Callback); /* 定时器中断回调注册 */

	API_TIM_Init(API_TIM1, 1U); /* 定时器初始化：API_TIM1，每 1ms 触发一次更新中断 */

	while (1)
	{
		usart_printf(USART1, "Timer_Bsp_t: %lu\r\n", Timer_Bsp_t);
		Enroll_LED_Control(LED1, LED_HIGH);
		Delay_ms(500U);
		Enroll_LED_Control(LED1, LED_LOW);
		Delay_ms(500U);
	}
}

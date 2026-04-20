#include "Enroll.h"

#include "Delay.h"
#include "tim.h"
#include "usart.h"

#include "My_Usart/My_Usart.h"
#include "Control_Task/Control_Task.h"

int main(void)	
{
	Enroll_LED_Init(LED_LOW); 				/*LED 资源注册*/
	Enroll_USART_Register(); 				/* USART 资源注册 */
	API_USART_Init(API_USART1, 115200); 	/* 串口初始化：API_USART1，波特率 115200。 */
	usart_printf(USART1, "USART1 OK\r\n");
	API_TIM_Init(API_TIM3, 1U); 			/* 定时器初始化：API_TIM3，每 1ms 触发一次更新中断。 */
	
	while(1)
	{
	/*
	printf节拍调试-50ms:
	*/	
		if (print_task_flag)
		{
			print_task_flag = 0;
			usart_printf(USART1, "USART_1_RX: %c\r\n", USART_1_RX);
			// printf("Time: %d\r\n", Timer_Bsp_t);
		}
	}
}

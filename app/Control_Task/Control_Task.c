#include "Control_Task.h"

#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"
// #include "Control/Control.h"

// #include "MPU6050_Int.h"

/* 程序运行的时间戳（s） */
uint32_t Timer_Bsp_t = 0;

/* printf节拍-50ms */
volatile uint8_t print_task_flag = 0;

/* 串口1接收数据 */
uint32_t USART_1_RX = 0;

/*
 * 定时器回调函数：
 * 由 API_TIM 的通用中断分发层在更新中断到来后调用。
 */
void Control_Task_TIM_Callback(API_TIM_Id_t id)
{
	static uint16_t time_t = 0U; /* 程序运行时间计数 */
	static uint8_t printf_50ms = 0U; /* 50ms printf 节拍计数 */
	static uint8_t pid_2ms_tick = 0U; /* 2ms PID 节拍计数 */

	if (id != API_TIM1)
	{
		return;
	}

	// Key_Tick();
	printf_50ms++;
	time_t++;
	pid_2ms_tick++;

	if (pid_2ms_tick >= 2U)
	{
		pid_2ms_tick = 0U;
		// pid_task_flag = 1U;
	}

	if (printf_50ms >= 50U)
	{
		printf_50ms = 0U;
		print_task_flag = 1U;
	}

	if (time_t >= 1000U)
	{
		time_t = 0U;
		Timer_Bsp_t++; /* 每 1s 更新一次全局时间戳 */
	}
}

/* 串口中断回调：由 API_USART 的通用分发层按 id 调用。 */
void Control_Task_USART_Callback(API_USART_Id_t id)
{
	uint32_t data;
	uint8_t rxValid;

	if (id != API_USART1)
	{
		return;
	}

	data = 0U;
	rxValid = 0U;
	usart_irq_dispatch_by_id(id, &data, &rxValid);
	if (rxValid != 0U)
	{
		USART_1_RX = data;
	}
}

// /*
// 	MPU6050外部中断服务函数
//  */
// void EXTI9_5_IRQHandler(void)
// {
// 	if (SYS_EXTI_IRQHandlerGroup(5U, 9U) != 0U)
// 	{
// 		mpu_flag = 1U;
// 	}
// }

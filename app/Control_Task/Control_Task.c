#include "Control_Task.h"

#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"
#include "Control/Control.h"

/* 程序运行的时间戳（s） */
uint32_t Timer_Bsp_t = 0;

/* printf节拍 */
volatile uint8_t print_task_flag = 0;

/* 编码器节拍 */
volatile uint8_t Encoder_flag = 0;

/* 串口接收数据 */
uint32_t USART_1_RX = 0;
uint32_t USART_2_RX = 0;
uint32_t USART_3_RX = 0;

/*
 * 定时器回调函数：
 * 由 API_TIM 的通用中断分发层在更新中断到来后调用。
 */
void Control_Task_TIM_Callback(API_TIM_Id_t id)
{
	static uint16_t time_t = 0U; /* 程序运行时间计数 */
	static uint8_t printf_tick = 0U; /* printf 节拍计数 */
	static uint8_t pid_2ms_tick = 0U; /* 2ms PID 节拍计数 */
	static uint8_t Encoder_tick = 0u; /* 编码器节拍数 */

	if (id != API_TIM1)
	{
		return;
	}

	Key_Tick(); /* 按键扫描函数，更新按键状态和事件 */

	pid_2ms_tick++;
	Encoder_tick++;
	printf_tick++;
	time_t++;

/* PID */
	if (pid_2ms_tick >= 2U)
	{
		pid_2ms_tick = 0U;
		pid_task_flag = 1U;
	}

/* 编码器 */
	if(Encoder_tick >= 20)
	{
		Encoder_tick = 0U;
		Encoder_flag = 1U;
	}

/* printf */
	if (printf_tick >= 50U)
	{
		printf_tick = 0U;
		print_task_flag = 1U;
	}

/* 时间戳 */
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
	data = 0U;
	rxValid = 0U;
	usart_irq_dispatch_by_id(id, &data, &rxValid);
	if (rxValid != 0U)
	{
		if (id == API_USART1)
		{
			USART_1_RX = data;
		}
		else if (id == API_USART2)
		{
			USART_2_RX = data;
		}
		else if (id == API_USART3)
		{
			USART_3_RX = data;
		}
	}
}

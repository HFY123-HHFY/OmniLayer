#include "Control_Task.h"

#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"
#include "Control/Control.h"
#include "KEY.h"
#include "Encoder.h"

/* 程序运行的时间戳（s） */
uint32_t Timer_Bsp_t = 0;

/* printf节拍 */
volatile uint8_t print_task_flag = 0;

/* 编码器节拍 */
volatile uint8_t Encoder_flag = 0;

/* 串口数据包解析结果缓存（新包到达时自动刷新） */
int16_t USART_Packet_Data[USART_PACKET_DATA_LEN] = {0};
uint8_t USART_Packet_Count = 0;

/*
 * 定时器回调函数：
 * 由 API_TIM 的通用中断分发层在更新中断到来后调用。
 * API_TIM1: 1ms -> PID 2ms
 */
void Control_Task_TIM_Callback(API_TIM_Id_t id)
{
	static uint8_t pid_2ms_tick = 0U;

	if (id != API_TIM1)
	{
		return;
	}

	pid_2ms_tick++;

	if (pid_2ms_tick >= 2U)
	{
		pid_2ms_tick = 0U;
		pid_task_flag = 1U;
	}
}

/*
 * API_TIM2: 1ms -> Encoder 20ms
 */
void Control_Task_Encoder_Callback(API_TIM_Id_t id)
{
	static uint8_t Encoder_tick = 0U;

	if (id != API_TIM2)
	{
		return;
	}

	Encoder_tick++;

	if (Encoder_tick >= 20)
	{
		Encoder_tick = 0U;
#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
		G3507_Encoder_SnapshotAll();
#endif
		Encoder1_Speed = API_Encoder_GetSpeed(API_ENCODER_1);
		Encoder2_Speed = API_Encoder_GetSpeed(API_ENCODER_2);
		Encoder_flag = 1U;
	}
}

/*
 * API_TIM3: 1ms -> Key + printf + time
 */
void Control_Task_Housekeeping_Callback(API_TIM_Id_t id)
{
	static uint8_t printf_tick = 0U;
	static uint16_t time_t = 0U;

	if (id != API_TIM3)
	{
		return;
	}

	Key_Tick();

	printf_tick++;
	time_t++;

	if (printf_tick >= 50U)
	{
		printf_tick = 0U;
		print_task_flag = 1U;
	}

	if (time_t >= 1000U)
	{
		time_t = 0U;
		Timer_Bsp_t++;
	}
}

/*
 * USART 中断回调协议解析：
 * 协议格式：s12,-34,56e
 */
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
			usart_Dispose_Data(USART1, &USART_DataTypeStruct, (uint8_t)data);
		}
	}
}

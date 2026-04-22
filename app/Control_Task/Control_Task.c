#include "Control_Task.h"

#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"

#include "MPU6050_Int.h"

/* 程序运行的时间戳（s） */
uint32_t Timer_Bsp_t = 0;

/* printf节拍-50ms */
volatile uint8_t print_task_flag = 0;

/* 串口1接收数据 */
uint32_t USART_1_RX = 0;

/*
 * TIM3 中断服务函数：
 * 1) 判断 TIM3 是否发生更新中断；
 * 2) 清除更新中断标志；
 * 3) 执行按键节拍与系统计时。
 */
void TIM3_IRQHandler(void)
{
	static uint16_t time_t = 0U; /* 程序运行时间计数 */
	static uint8_t printf_50ms = 0U; /* 50ms printf 节拍计数 */

	if ((TIM3->SR & TIM_SR_UIF) == 0U)
	{
		return;
	}

	/* 先清除更新中断标志，避免重复进中断。 */
	TIM3->SR &= ~TIM_SR_UIF;

	Key_Tick();
	printf_50ms++;
	time_t++;

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

/*
 * USART1 中断服务函数：
 * 1) RXNE 读取接收数据；
 * 2) TXE 调用串口发送中断处理。
 */
void USART1_IRQHandler(void)
{
	static uint32_t data = 0U;

	if ((USART1->SR & USART_SR_RXNE) != 0U)
	{
		data = USART1->DR;
		USART_1_RX = data;
	}

	if (((USART1->SR & USART_SR_TXE) != 0U) &&
		((USART1->CR1 & USART_CR1_TXEIE) != 0U))
	{
		usart_tx_irq_handler(USART1);
	}
}

/*
	MPU6050外部中断服务函数
 */
void EXTI9_5_IRQHandler(void)
{
	if (SYS_EXTI_IRQHandlerGroup(5U, 9U) != 0U)
	{
		mpu_flag = 1U;
	}
}

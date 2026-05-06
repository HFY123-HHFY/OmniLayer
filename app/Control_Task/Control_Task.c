#include "Control_Task.h"

#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"
#include "Control/Control.h"

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
	static uint8_t pid_2ms_tick = 0U; /* 2ms PID 节拍计数 */

	if ((TIM3->SR & TIM_SR_UIF) == 0U)
	{
		return;
	}

	/* 先清除更新中断标志，避免重复进中断。 */
	TIM3->SR &= ~TIM_SR_UIF;

	Key_Tick();
	printf_50ms++;
	time_t++;
	pid_2ms_tick++;

	if (pid_2ms_tick >= 2U)
	{
		pid_2ms_tick = 0U;
		pid_task_flag = 1U;
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

/*
 * USART1 中断服务函数（IRQ Handler）工作流程：
 *
 * 第1个 if（RXNE 分支）：
 * - 含义：接收数据寄存器非空，说明“收到新字节”。
 * - 动作：调用 MyUsart_RtosRxIrqHandler(USART1)，在 ISR 中把该字节投递到
 *   RTOS 接收链路（RX字节队列 + RX信号量）。
 * - 目的：中断只做最小搬运，不做命令解析，解析放到任务里执行。
 *
 * 第2个 if（TXE + TXEIE 分支）：
 * - 含义：发送数据寄存器空，并且已开启 TXE 中断。
 * - 动作：调用 usart_tx_irq_handler(USART1) 继续从发送缓冲取字节发出。
 * - 目的：把发送动作拆成“后台中断搬运”，避免阻塞业务任务。
 */
void USART1_IRQHandler(void)
{
	/* 判断是否收到新字节 */
	if ((USART1->SR & USART_SR_RXNE) != 0U)
	{
		/* RXNE=1：有新接收字节，交给 RTOS 串口接收封装（ISR -> Queue/Sem）*/
		/* 把字节从 ISR 投递到 RTOS 接收链路。 */
		MyUsart_RtosRxIrqHandler(USART1);
	}

	/* 判断是否发送寄存器空，并且发送中断使能 */
	if (((USART1->SR & USART_SR_TXE) != 0U) &&
		((USART1->CR1 & USART_CR1_TXEIE) != 0U))
	{
		/* TXE=1 且 TXEIE=1：发送寄存器空，继续发送下一字节 */
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

#include "Control_Task.h"

#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"
#include "Control/Control.h"
#include "FreeRTOS.h"
#include "task.h"

#include "MPU6050_Int.h"

/* 程序运行的时间戳（s） */
uint32_t Timer_Bsp_t = 0;

/*
 * TIM3 对应的周期服务任务句柄：
 * - 由 RTOS 任务层注册；
 * - TIM3 IRQ 每 1ms 给该任务发送一次通知；
 * - ISR 不再负责软件分频和业务逻辑。
 */
static TaskHandle_t s_periodicTaskHandle = NULL;

void ControlTask_RegisterTimerTickTarget(TaskHandle_t periodicTaskHandle)
{
	s_periodicTaskHandle = periodicTaskHandle;
}

/*
 * TIM3 中断服务函数：
 * 1) 判断 TIM3 是否发生更新中断；
 * 2) 清除更新中断标志；
 * 3) 仅向 RTOS 周期任务发送 1ms 节拍通知。
 */
void TIM3_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE; /* 用于记录是否需要切换到更高优先级任务 */

	/* 判断 TIM3 是否发生更新中断 */
	if ((TIM3->SR & TIM_SR_UIF) == 0U)
	{
		return;
	}

	/* 先清除更新中断标志，避免重复进中断。 */
	TIM3->SR &= ~TIM_SR_UIF;

	/*
	 * FreeRTOS 分支约束：TIM3 只提供最小 1ms 节拍通知。
	 * 所有 2ms/50ms/1s 软件分频都放在任务里做，避免 ISR 膨胀。
	 */
	if (s_periodicTaskHandle != NULL)
	{
		vTaskNotifyGiveFromISR(s_periodicTaskHandle, &xHigherPriorityTaskWoken);
	}

	/* 若本次中断唤醒了更高优先级任务，则请求立刻切换。 */
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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

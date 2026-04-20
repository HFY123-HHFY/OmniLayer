#include "Control_Task.h"

#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"

uint32_t Timer_Bsp_t = 0; // 程序运行的时间戳（s）
volatile uint8_t print_task_flag = 0; // printf节拍-50ms
uint32_t USART_1_RX = 0;

/*
 * TIM3 中断服务函数：
 * 1) 判断 TIM3 是否发生更新中断；
 * 2) 清除更新中断标志；
 * 3) 业务逻辑
 */
void TIM3_IRQHandler(void)
{
	static uint16_t time_t = 0; //程序运行时间计数
	static uint8_t printf_50ms = 0; // 50ms printf节拍计数

	if ((TIM3->SR & TIM_SR_UIF) == 0U)
	{
		return;
	}
	/* 先清除更新中断标志，避免重复进中断。 */
	TIM3->SR &= ~TIM_SR_UIF;

	printf_50ms++;
	time_t++;

/*
printf节拍-50ms
*/
        if (printf_50ms >= 50)
        {
            printf_50ms = 0;
            print_task_flag = 1;
        }
 /*
程序运行时间
*/  		
	if (time_t >= 1000U)
	{
		time_t = 0U;
		Timer_Bsp_t++; //每 1s 更新一次全局时间戳
	}
}

/*
 * USART1 中断服务函数：
 * 1) 判断是否收到 1 个字节；
 * 2) 读取 DR 清除 RXNE；
 * 3) 回显收到的字节，方便串口助手确认收发都正常。
 */
void USART1_IRQHandler(void)
{
	static uint32_t Data = 0;

	if ((USART1->SR & USART_SR_RXNE) != 0U)
	{
		Data = USART1->DR; /* 读取 DR 清除 RXNE。 */
		USART_1_RX = Data;
	}
	if (((USART1->SR & USART_SR_TXE) != 0U) && ((USART1->CR1 & USART_CR1_TXEIE) != 0U))
	{
		usart_tx_irq_handler(USART1);
	}
}

#ifndef __CONTROL_TASK_H
#define __CONTROL_TASK_H

#include <stdint.h>
#include "tim.h"
#include "usart.h"

extern uint32_t Timer_Bsp_t; // 程序运行的时间戳（s）
extern volatile uint8_t print_task_flag; // printf节拍-50ms
extern volatile uint8_t Encoder_flag; // 编码器节拍

extern uint32_t USART_1_RX; // USART1 接收的最新数据
extern uint32_t USART_2_RX; // USART2 接收的最新数据
extern uint32_t USART_3_RX; // USART3 接收的最新数据

void Control_Task_TIM_Callback(API_TIM_Id_t id);
void Control_Task_USART_Callback(API_USART_Id_t id);

#endif /* __CONTROL_TASK_H */

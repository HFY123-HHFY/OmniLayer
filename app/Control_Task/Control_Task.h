#ifndef __CONTROL_TASK_H
#define __CONTROL_TASK_H

#include <stdint.h>
#include "tim.h"
#include "usart.h"

extern uint32_t Timer_Bsp_t; // 程序运行的时间戳（s）
extern volatile uint8_t print_task_flag; // printf节拍-50ms
extern volatile uint8_t Encoder_flag; // 编码器节拍

/* 串口数据包解析结果缓存（全局可读，新包到达时自动刷新） */
#define USART_PACKET_DATA_LEN 10U
extern int16_t USART_Packet_Data[USART_PACKET_DATA_LEN];
extern uint8_t USART_Packet_Count;

void Control_Task_TIM_Callback(API_TIM_Id_t id);           /* TIM1: PID 控制节拍 */
void Control_Task_Encoder_Callback(API_TIM_Id_t id);       /* TIM2: 编码器读取节拍 */
void Control_Task_Housekeeping_Callback(API_TIM_Id_t id);  /* TIM3: 按键/printf/时间戳 */
void Control_Task_USART_Callback(API_USART_Id_t id);

#endif /* __CONTROL_TASK_H */

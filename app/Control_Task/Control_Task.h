#ifndef __CONTROL_TASK_H
#define __CONTROL_TASK_H

#include <stdint.h>

extern uint32_t Timer_Bsp_t; // 程序运行的时间戳（s）
extern volatile uint8_t print_task_flag; // printf节拍-50ms
extern uint32_t USART_1_RX; // USART1 接收的最新数据

#endif /* __CONTROL_TASK_H */

#ifndef __CONTROL_TASK_H
#define __CONTROL_TASK_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

extern uint32_t Timer_Bsp_t; // 程序运行的时间戳（s）

/*
 * 注册 TIM3 的 1ms 节拍任务句柄。
 *
 * 设计约束：
 * 1) FreeRTOS 分支不再兼容裸机 flag 传递；
 * 2) TIM3 IRQ 只负责发 1ms 节拍通知；
 * 3) 2ms PID、50ms 遥测、1s 时间戳等软件分频统一放到任务里完成。
 *
 * 参数：
 * - periodicTaskHandle: 接收 1ms 节拍的周期服务任务句柄
 */
void ControlTask_RegisterTimerTickTarget(TaskHandle_t periodicTaskHandle);

#endif /* __CONTROL_TASK_H */

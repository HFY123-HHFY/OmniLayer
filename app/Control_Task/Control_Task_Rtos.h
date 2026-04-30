#ifndef __CONTROL_TASK_RTOS_H
#define __CONTROL_TASK_RTOS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 创建最小 FreeRTOS 验证任务：
 * 1) LED1: 1000ms 翻转
 * 2) LED2: 500ms 翻转
 */
void ControlTask_RtosCreate(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONTROL_TASK_RTOS_H */

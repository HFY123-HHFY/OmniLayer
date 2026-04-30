#ifndef __CONTROL_TASK_RTOS_H
#define __CONTROL_TASK_RTOS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RTOS 任务层入口：
 * 1) 创建系统初始化任务（一次性执行后自删除）
 * 2) 创建 LED1/LED2 周期闪烁任务
 */
void ControlTask_RtosCreate(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONTROL_TASK_RTOS_H */

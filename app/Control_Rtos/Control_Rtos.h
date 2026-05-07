#ifndef __CONTROL_RTOS_H
#define __CONTROL_RTOS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RTOS 应用任务编排入口：
 * 1) 创建初始化任务（一次性）
 * 2) 初始化任务再拉起发送/接收业务任务（常驻）
 */
void ControlRtos_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONTROL_RTOS_H */

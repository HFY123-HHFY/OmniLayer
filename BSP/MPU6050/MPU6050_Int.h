#ifndef __MPU6050_INT_H
#define __MPU6050_INT_H

#include <stdint.h>

#include "sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 按指定端口/引脚初始化 MPU6050 EXTI。 */
void MPU6050_EXTI_Init(void *port, uint16_t pin, SYS_EXTI_Trigger_t trigger,
	uint8_t preemptPriority, uint8_t subPriority);

/* 按板级映射默认策略初始化 MPU6050 EXTI。 */
void MPU6050_EXTI_InitBoard(void *port, uint16_t pin);

/* 供 MCU it 文件调用：处理某个 EXTI 线组（如 5~9 或 10~15）。 */
void MPU6050_EXTI_IRQHandlerGroup(uint8_t startLine, uint8_t endLine);

extern float Pitch, Roll, Yaw;	        /* Pitch：俯仰角，Roll：横滚角，Yaw：偏航角 */ 
extern short gyrox, gyroy, gyroz;       /*         角速度,x轴、y轴、z轴            */
extern short aacx, aacy, aacz;          /*        加速度 ,x轴、y轴、z轴           */

extern uint8_t mpu_flag;                /* mpu6050中断标志位 */
void mpu_angle(void);                   /* 获取 MPU6050 角度信息 */

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_INT_H */

#ifndef __CONTROL_H
#define __CONTROL_H

#include <stdint.h>

#include "PID/PID.h"
#include "Filter/Filter.h"
#include "MPU6050_Int.h"
#include "pwm.h"
#include "TB6612.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ±2000dps 量程下 MPU6050 陀螺灵敏度：16.4 LSB/(deg/s) */
#define GYRO_SENS_2000DPS (16.4f)

/* PID 输出加载到电机前的总限幅。 */
#define MOTOR_MIX_LIMIT (2047.0f)

/* PID 任务节拍标志：由定时中断置位。 */
extern uint8_t pid_task_flag;

/* 目标姿态/高度。 */
extern float Target_Pitch;
extern float Target_Roll;
extern float Target_Yaw;

/* 外环 PID：角度。 */
extern PID_TypeDef pid_pitch;
extern PID_TypeDef pid_roll;
extern PID_TypeDef pid_yaw;

/* 内环 PID：角速度。 */
extern PID_TypeDef pid_rate_pitch;
extern PID_TypeDef pid_rate_roll;
extern PID_TypeDef pid_rate_yaw;

/* 速度环 */
extern PID_EncoderSpeed_t speed_loop;

/*
 * 控制初始化：
 * 1) 初始化外环/内环 PID
 * 2) 初始化传感器低通滤波器
 * 3) 设置串级控制对象
 */
void PID_Contorl_Init(void);

/* 设置陀螺零偏（单位：原始 LSB）。 */
void Set_Gyro_Bias(float bias_x, float bias_y, float bias_z);

/* 速度环初始化。 */
void PID_Speed_Init(void);

/*
 * Pitch/Roll 串级 PID 控制。
*/
void PID_Pitch_Roll_Combined(float actual_pitch, float actual_roll);

/* 
 * 速度环控制函数 
*/
void PID_Speed_Control(float actual_left, float actual_right);

/*
 * 预留电机加载接口。
 * 当前默认实现仅做占位，方便后续接真实电机混控。
 */
void Motor_Test(void);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_H */

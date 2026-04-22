#include "MPU6050_Int.h"
#include "MPU6050.h"

/*
    * MPU6050 外部中断处理代码
     * 1) 通过 Enroll 层注册 EXTI 线资源，优先级由 sys.c 统一管理；
     * 2) 中断服务函数调用 MPU6050_EXTI_IRQHandlerGroup，根据线号分组处理。
     * 3) 中断事件通过全局变量 MPU6050_IntFlag 通知上层任务。
     * 注意：MPU6050 的 INT 引脚默认是低电平有效的，因此建议配置为上升沿触发。

 */

float Pitch = 0.0f, Roll = 0.0f, Yaw = 0.0f;	        /* Pitch：俯仰角，Roll：横滚角，Yaw：偏航角 */ 
short gyrox = 0, gyroy = 0, gyroz = 0;      /*         角速度,x轴、y轴、z轴            */
short aacx = 0, aacy = 0, aacz = 0;          /*        加速度 ,x轴、y轴、z轴           */
/*short短整型，16位有符号整数，范围-32768~32767，单位：m/s^2, %hd*/

/* mpu6050 中断标志位 */
uint8_t mpu_flag = 0U;

#define MPU6050_EXTI_DEFAULT_TRIGGER        SYS_EXTI_TRIGGER_RISING // 上升沿触发
#define MPU6050_EXTI_DEFAULT_PREEMPT_PRIO   (0U)
#define MPU6050_EXTI_DEFAULT_SUB_PRIO       (2U)

// 按指定端口/引脚初始化 MPU6050 EXTI。
void MPU6050_EXTI_Init(void *port, uint16_t pin, SYS_EXTI_Trigger_t trigger,
	uint8_t preemptPriority, uint8_t subPriority)
{
	SYS_EXTI_Register(port, pin, trigger, preemptPriority, subPriority);
}

// 按板级映射默认策略初始化 MPU6050 EXTI。
void MPU6050_EXTI_InitBoard(void *port, uint16_t pin)
{
	MPU6050_EXTI_Init(port,
		pin,
		MPU6050_EXTI_DEFAULT_TRIGGER,
		MPU6050_EXTI_DEFAULT_PREEMPT_PRIO,
		MPU6050_EXTI_DEFAULT_SUB_PRIO);
}

void MPU6050_EXTI_IRQHandlerGroup(uint8_t startLine, uint8_t endLine)
{
	(void)SYS_EXTI_IRQHandlerGroup(startLine, endLine);
}

/* 获取 MPU6050 角度信息 */
void mpu_angle(void)
{
	if (mpu_flag == 1U)
	{
		mpu_flag = 0U;
		mpu_dmp_get_data(&Pitch, &Roll, &Yaw);
		// MPU_Get_Gyroscope(&gyrox, &gyroy, &gyroz);
		/* MPU_Get_Accelerometer(&aacx, &aacy, &aacz); */
	}
}

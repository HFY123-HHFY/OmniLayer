/* Enroll 注册层：负责把板级资源注册到 BSP。 */
#include "Enroll.h"

/* 系统层：统一系统初始化入口。 */
#include "sys.h"

/* FreeRTOS 内核接口。 */
#include "FreeRTOS.h"
#include "task.h"

/* app 任务层：最小 FreeRTOS 任务创建入口。 */
#include "Control_Task/Control_Task_Rtos.h"

int main(void)
{
	/*
	 * main 在 RTOS 架构下只做两件事：
	 * 1) 板级最小初始化（这里先保留 LED/SYS）
	 * 2) 创建任务并启动调度器
	 *
	 * 业务循环不再写在 main 的 while(1)，而是写到各个任务函数里。
	 */
	Enroll_LED_Init(LED_LOW);
	SYS_Init();

	/* 创建任务并启动调度器。 */
	ControlTask_RtosCreate();
	vTaskStartScheduler();

	/*
	 * 理论上不会到这里，除非堆内存不足或任务创建失败。
	 * 正常运行时 CPU 会在 FreeRTOS 调度器里切换 Task1/Task2。
	 */
	for (;;)
	{
	}
}

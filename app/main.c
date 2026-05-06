/* Enroll 注册层：负责把板级资源注册到 BSP */
#include "Enroll.h"

/* FreeRTOS 内核接口 */
#include "FreeRTOS.h"	/* FreeRTOS 基础定义 */
#include "task.h" 		/* 任务管理接口 */

/* app 任务层：最小 FreeRTOS 任务创建入口 */
#include "Control_Task/Control_Task_Rtos.h"

int main(void)
{
	ControlTask_RtosCreate(); /* 创建 app 任务 */
	vTaskStartScheduler(); /* 启动调度器 */

	for (;;)
	{
	}
}

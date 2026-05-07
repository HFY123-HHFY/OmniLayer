/* Enroll 注册层：负责把板级资源注册到 BSP */
#include "Enroll.h"

/* FreeRTOS 内核接口 */
#include "FreeRTOS.h"	/* FreeRTOS 基础定义 */
#include "task.h" 		/* 任务管理接口 */

/* app 任务层：RTOS 任务编排入口 */
#include "Control_Rtos/Control_Rtos.h"

int main(void)
{
	ControlRtos_Create(); /* 创建 app 任务 */
	vTaskStartScheduler(); /* 启动调度器 */

	for (;;)
	{
	}
}

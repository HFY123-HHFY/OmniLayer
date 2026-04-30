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
	 * 最小 RTOS 验证仅保留 LED 资源初始化，
	 * 其它外设初始化留待后续逐步迁移到任务模型。
	 */
	Enroll_LED_Init(LED_LOW);
	SYS_Init();

	/* 创建任务并启动调度器。 */
	ControlTask_RtosCreate();
	vTaskStartScheduler();

	/* 理论上不会到这里，除非堆或任务创建失败。 */
	for (;;)
	{
	}
}

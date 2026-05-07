#include "Control_Rtos.h"

#include "FreeRTOS.h"
#include "task.h"

#include "Enroll.h"
#include "LED.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"

/* 任务参数：栈大小单位为 word */
#define TASK_INIT_STACK_WORDS      (256U)
#define TASK_APP_STACK_WORDS       (256U)
#define TASK_ECHO_STACK_WORDS      (256U)

/* 任务优先级 数字越大优先级越高 */
#define TASK_INIT_PRIORITY         (tskIDLE_PRIORITY + 3U)
#define TASK_APP_PRIORITY          (tskIDLE_PRIORITY + 1U)
#define TASK_ECHO_PRIORITY         (tskIDLE_PRIORITY + 1U)

/* 串口1周期发送示例数据 */
static void TaskAppProducer(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        (void)MyUsart_RtosSendText("HFY\r\n");
        (void)MyUsart_RtosPrintf("%.2f\r\n", 3.14f);

        vTaskDelay(pdMS_TO_TICKS(3000U));
    }
}

/* 串口1数据接收-回显任务 */
static void TaskEchoHandler(void *pvParameters)
{
    char line[64];

    (void)pvParameters;

    for (;;)
    {
        if (MyUsart_RtosRecvLine(line, sizeof(line), 1000U) == 0U)
        {
            continue;
        }
        /* 回显接收到的数据 */
        (void)MyUsart_RtosPrintf("Data: %s\r\n", line);
    }
}

/* 一次性初始化任务 */
static void TaskInit(void *pvParameters)
{
    (void)pvParameters;

    /* 注册LED 并设置初始状态 0*/
    Enroll_LED_Init(LED_LOW);
    /* 注册 USART */
    Enroll_USART_Register();

    API_USART_Init(API_USART1, 115200U); /* 初始化 USART1，波特率 115200 */

    /* 启动 USART1 的 RTOS 驱动 */
    if (MyUsart_RtosStart(USART1) == 0U)
    {
        vTaskDelete(0);
    }             

    /* 创建应用任务 */
    (void)xTaskCreate(TaskAppProducer,
                      "app_prod",
                      TASK_APP_STACK_WORDS,
                      0,
                      TASK_APP_PRIORITY,
                      0);

    /* 创建回显任务 */
    (void)xTaskCreate(TaskEchoHandler,
                      "echo_hdl",
                      TASK_ECHO_STACK_WORDS,
                      0,
                      TASK_ECHO_PRIORITY,
                      0);

    /* 删除初始化任务 */
    vTaskDelete(0);
}

void ControlRtos_Create(void)
{
    (void)xTaskCreate(TaskInit,
                      "init",
                      TASK_INIT_STACK_WORDS,
                      0,
                      TASK_INIT_PRIORITY,
                      0);
}

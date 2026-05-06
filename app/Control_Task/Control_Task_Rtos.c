#include "Control_Task_Rtos.h"

#include "FreeRTOS.h"
#include "task.h"

#include "Enroll.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"

#include <string.h>

/*
 * 任务参数说明：
 * 1) STACK_WORDS 是任务栈大小，单位是 word，不是 byte。
 * 2) PRIORITY 数字越大优先级越高。
 */
#define TASK_INIT_STACK_WORDS      (256U)
#define TASK_APP_STACK_WORDS       (256U)
#define TASK_ECHO_STACK_WORDS      (256U)

#define TASK_INIT_PRIORITY         (tskIDLE_PRIORITY + 3U)
#define TASK_APP_PRIORITY          (tskIDLE_PRIORITY + 1U)
#define TASK_ECHO_PRIORITY         (tskIDLE_PRIORITY + 1U)

/*
 * 发送示例任务：持续演示如何在“业务任务”里使用 RTOS 串口发送接口。
 *
 * 这里每 3 秒发送一组数据：
 * 1) 整数文本：123
 * 2) ASCII 字符串：Hfy
 * 3) UTF-8 字符串：胡
 * 4) 浮点文本：3.14
 *
 * 目的不是发固定命令，而是演示：
 * 业务任务可以一直调用 MyUsart_RtosSendText / MyUsart_RtosPrintf。
 */
static void TaskAppProducer(void *pvParameters)
{
    uint32_t tickCount;

    (void)pvParameters;
    tickCount = 0U;

    for (;;)
    {
        (void)MyUsart_RtosPrintf("[demo][send] int text: %d\r\n", 123);
        (void)MyUsart_RtosSendText("[demo][send] ascii text: Hfy\r\n");
        (void)MyUsart_RtosSendText("[demo][send] utf8 text: 胡\r\n");
        (void)MyUsart_RtosPrintf("[demo][send] float text: %.2f\r\n", 3.14f);
        (void)MyUsart_RtosPrintf("[app] alive %lu\r\n", (unsigned long)tickCount);

        tickCount++;
        vTaskDelay(pdMS_TO_TICKS(3000U));
    }
}

/*
 * 接收回显任务：
 * 1) 调用 MyUsart_RtosRecvLine 等待接收一整行文本；
 * 2) 只要串口助手发来任意内容（以回车结尾），这里都会收到；
 * 3) 收到后原样回显出来，不再写死固定命令匹配。
 *
 * 例如你发送：
 * 1) 123
 * 2) Hfy
 * 3) 胡
 * 4) 3.14
 *
 * 都会显示成：
 * [demo][recv] echo: 你发送的内容
 */
static void TaskEchoHandler(void *pvParameters)
{
    char line[64];

    (void)pvParameters;
    (void)memset(line, 0, sizeof(line));

    for (;;)
    {
        if (MyUsart_RtosRecvLine(line, sizeof(line), 1000U) == 0U)
        {
            continue;
        }

        if (strcmp(line, "help") == 0)
        {
            (void)MyUsart_RtosSendText("send any line, I will echo it back. examples: 123 | Hfy | 胡 | 3.14\r\n");
        }
        else
        {
            (void)MyUsart_RtosPrintf("[demo][recv] echo: %s\r\n", line);
        }
    }
}

/*
 * 初始化任务：
 * 1) 资源注册；
 * 2) 外设初始化；
 * 3) 启动 My_Usart 的 RTOS 串口封装；
 * 4) 创建业务任务；
 * 5) 完成后自删除。
 *
 * 为什么初始化任务只运行一次，但串口收发还能一直工作？
 * 1) 因为初始化任务本来就只负责“一次性准备工作”；
 * 2) MyUsart_RtosStart 内部会创建 uart_tx / uart_rx 两个常驻后台任务；
 * 3) 本文件又创建了 TaskAppProducer / TaskEchoHandler 两个业务任务；
 * 4) 所以即使 TaskInit 自删了，后面这些常驻任务仍会一直运行。
 */
static void TaskInit(void *pvParameters)
{
    (void)pvParameters;

    /* 板子注册层初始化。 */
    Enroll_USART_Register();

    /* API 层串口初始化。 */
    API_USART_Init(API_USART1, 115200U);

    /* 启动 My_Usart 里的 RTOS 串口封装。 */
    if (MyUsart_RtosStart(USART1) == 0U)
    {
        vTaskDelete(0);
    }

    (void)MyUsart_RtosSendText("RTOS USART demo ready. type: help for usage\r\n");

    /* 创建长期运行的业务任务。 */
    (void)xTaskCreate(TaskAppProducer,
                      "app_prod",
                      TASK_APP_STACK_WORDS,
                      0,
                      TASK_APP_PRIORITY,
                      0);

    (void)xTaskCreate(TaskEchoHandler,
                      "echo_hdl",
                      TASK_ECHO_STACK_WORDS,
                      0,
                      TASK_ECHO_PRIORITY,
                      0);

    /* 初始化完成后删除自身。 */
    vTaskDelete(0);
}

/* 任务层入口：先创建初始化任务，后续由初始化任务拉起常驻业务任务。 */
void ControlTask_RtosCreate(void)
{
    (void)xTaskCreate(TaskInit,
                      "init",
                      TASK_INIT_STACK_WORDS,
                      0,
                      TASK_INIT_PRIORITY,
                      0);
}

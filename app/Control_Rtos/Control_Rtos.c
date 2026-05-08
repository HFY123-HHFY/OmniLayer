#include "Control_Rtos.h"

#include "FreeRTOS.h"
#include "task.h"

#include "Enroll.h"
#include "LED.h"
#include "KEY.h"
#include "tim.h"
#include "usart.h"
#include "My_Usart/My_Usart.h"
#include "Control_Task/Control_Task.h"
#include "Control/Control.h"
#include "MPU6050_Int.h"

#include <stdlib.h>
#include <string.h>

/* init 任务栈（单位：word），只执行一次后自删。 */
#define STK_INIT      (256U)
/* 周期任务栈（单位：word），承接 1ms 节拍并做软件分频。 */
#define STK_PERIODIC  (256U)
/* 控制台任务栈（单位：word），用于串口回显/后续命令交互。 */
#define STK_CONSOLE   (256U)
/* 遥测任务栈（单位：word），专门做低优先级调试打印。 */
#define STK_TELEM     (256U)

/* init 任务优先级：高于常驻任务，确保启动阶段尽快完成。 */
#define PRIO_INIT      (tskIDLE_PRIORITY + 3U)
/* 周期任务优先级：高于串口任务，保证周期业务时效性。 */
#define PRIO_PERIODIC  (tskIDLE_PRIORITY + 2U)
/* 控制台任务优先级：中低优先级。 */
#define PRIO_CONSOLE   (tskIDLE_PRIORITY + 1U)
/* 遥测任务优先级：最低，避免打印影响控制周期。 */
#define PRIO_TELEM     (tskIDLE_PRIORITY + 1U)

/* TIM3 的 1ms 节拍统一唤醒这个周期服务任务。 */
static TaskHandle_t s_periodicServiceTaskHandle = NULL;
/* 周期任务每到 50ms 通知一次串口发送任务。 */
static TaskHandle_t s_telemetryTaskHandle = NULL;

/*
 * 串口-调试数据打印周期（毫秒）：
 * - 默认 50ms；
 * - 可通过控制台命令 `rate <ms>` 运行时动态修改。
 */
static volatile uint16_t s_telemPeriodMs = 50U;

/*
 * 串口控制台任务：
 * - 常驻等待接收一整行串口文本；
 * - 回显
 */
static void TaskConsoleService(void *pvParameters)
{
    char line[64]; /* 接收缓冲区，保存 USART RX 任务组包后的一行文本。 */
    char *endPtr; /* 用于 strtol 解析命令参数的辅助指针。 */
    long newRate; /* 从命令解析得到的新周期值（ms）。 */

    (void)pvParameters;

    for (;;)
    {
        /* 接收一行串口文本，如果超时则继续等待。 */
        if (MyUsart_RtosRecvLine(line, sizeof(line), 1000U) == 0U)
        {
            continue;
        }

        /*
         * 简易命令行：
         * 1) rate <ms>  动态设置遥测打印周期（范围 10~5000ms）；
         * 2) rate?      查询当前遥测周期；
         */
        if (strncmp(line, "rate ", 5U) == 0)
        {
            endPtr = NULL;
            newRate = strtol(&line[5], &endPtr, 10);

            if ((endPtr == &line[5]) || (newRate < 10L) || (newRate > 5000L))
            {
                (void)MyUsart_RtosPrintf("rate invalid, range: 10~5000 ms\r\n");
                continue;
            }

            s_telemPeriodMs = (uint16_t)newRate;
            (void)MyUsart_RtosPrintf("rate set to %u ms\r\n", (uint32_t)s_telemPeriodMs);
            continue;
        }

        if (strcmp(line, "rate?") == 0)
        {
            (void)MyUsart_RtosPrintf("rate=%u ms\r\n", (uint32_t)s_telemPeriodMs);
            continue;
        }
    }
}

/*
 * 低优先级-串口调试打印数据任务：
 * - 仅负责调试打印；
 * - 与周期任务解耦，避免打印行为拖慢周期业务。
 */
static void TaskTelemetryService(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /*
         * 阻塞等待周期任务发来的“该打印了”通知：
         * - pdTRUE：收到后自动清零通知计数；
         * - portMAX_DELAY：一直阻塞到有通知，不空转占用 CPU。
         */
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /*
         * 直接读取并打印目标变量。
         * 后续要看姿态角等数据时，直接在这里增加打印字段即可，
         * 不需要为每个变量再新增一个中转全局变量。
         */
        (void)MyUsart_RtosPrintf("time=%lus\r\n", Timer_Bsp_t);
    }
}

/*
 * 周期服务任务：
 * - 统一消费 TIM3 产生的 1ms 节拍；
 * - 在任务上下文里做软件分频；
 * - 当前托管 1ms 按键扫描、2ms 控制入口、50ms 状态节拍、1s 系统时间戳。
 */
static void TaskPeriodicService(void *pvParameters)
{
    uint8_t pid_Divider = 0U; /* PID 控制的分频器：每 2ms 执行一次 PID 计算 */
    uint16_t time_Divider = 0U; /* 系统时间戳的分频器：每 1000ms 更新一次系统时间戳 */
    uint16_t telemDivider = 0U; /* 串口打印节拍的分频器：每 s_telemPeriodMs ms 通知一次打印任务。 */
    uint16_t telemPeriod; /* 本次周期任务的打印节拍周期快照，单位 ms。 */

    (void)pvParameters;

    for (;;)
    {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* 1ms：按键扫描属于轻量周期动作，放在周期任务里即可。 */
        Key_Tick();

        /* 2ms：控制入口。后续接真实姿态解算/电机混控，从这里继续扩展。 */
        pid_Divider++;
        if (pid_Divider >= 2U)
        {
            pid_Divider = 0U;
            // PID_Pitch_Roll_Combined(Pitch, Roll);
        }

        /*
         * 串口打印节拍：
         * - 周期任务按 1ms 累计；
         * - 到达 s_telemPeriodMs 后通知打印任务执行一次打印。
         */
        telemPeriod = s_telemPeriodMs;
        if (telemPeriod < 10U)
        {
            telemPeriod = 10U;
        }
        telemDivider++;
        if (telemDivider >= telemPeriod)
        {
            telemDivider = 0U;
            if (s_telemetryTaskHandle != NULL)
            {
                /*
                 * xTaskNotifyGive：给串口任务的通知计数 +1，
                 * 等价于“告诉它：到打印周期了，可以执行一次输出”。
                 */
                xTaskNotifyGive(s_telemetryTaskHandle);
            }
        }

        /* 1s：系统时间戳。 */
        time_Divider++;
        if (time_Divider >= 1000U)
        {
            time_Divider = 0U;
            Timer_Bsp_t++;
        }
    }
}

/* 一次性初始化任务：只负责启动系统资源与常驻任务。 */
static void TaskSystemInit(void *pvParameters)
{
    (void)pvParameters;

    /* 注册板级 LED 资源并设置为默认灭灯状态。 */
    Enroll_LED_Init(LED_LOW);

    /* 注册串口资源并初始化 USART1。 */
    Enroll_USART_Register();
    API_USART_Init(API_USART1, 115200U);

    /* 初始化 TIM3 为 1ms 周期中断，这是整个周期任务的节拍源。 */
    API_TIM_Init(API_TIM3, 1U);

    /* 初始化PID*/
    PID_Contorl_Init();

    /* 启动 USART1 的 RTOS 驱动 */
    if (MyUsart_RtosStart(USART1) == 0U)
    {
        vTaskDelete(0);
    }             

    /* 创建串口控制台任务 */
    (void)xTaskCreate(TaskConsoleService,
                      "console",
                      STK_CONSOLE,
                      0,
                      PRIO_CONSOLE,
                      0);

    /* 创建低优先级遥测任务（专门调试打印）。 */
    (void)xTaskCreate(TaskTelemetryService,
                      "telem",
                      STK_TELEM,
                      0,
                      PRIO_TELEM,
                      &s_telemetryTaskHandle);

    /* 创建系统周期任务：它统一承接 TIM3 的 1ms 节拍。 */
    (void)xTaskCreate(TaskPeriodicService,
                      "periodic",
                      STK_PERIODIC,
                      0,
                      PRIO_PERIODIC,
                      &s_periodicServiceTaskHandle);

    /*
     * 把周期服务任务注册给 TIM3 中断。
     * 从这一步开始，TIM3 IRQ 每 1ms 只负责通知该任务一次。
     */
    ControlTask_RegisterTimerTickTarget(s_periodicServiceTaskHandle);

    /* 初始化任务只执行一次，完成后自删。 */
    vTaskDelete(0);
}

void ControlRtos_Create(void)
{
    (void)xTaskCreate(TaskSystemInit,
                      "sys_init",
                      STK_INIT,
                      0,
                      PRIO_INIT,
                      0);
}

#include "Control_Task_Rtos.h"

#include "FreeRTOS.h"
#include "task.h"

#include "Enroll.h"
#include "usart.h"

/*
 * 这三个宏是任务参数，不是业务逻辑：
 * 1) STACK_WORDS: 任务栈大小（单位是 word，不是 byte）
 * 2) PRIORITY: 任务优先级，数字越大优先级越高
 */
#define TASK_INIT_STACK_WORDS      (256U)
#define TASK_LED_STACK_WORDS       (128U)

#define TASK_INIT_PRIORITY         (tskIDLE_PRIORITY + 3U)
#define TASK_LED1_PRIORITY         (tskIDLE_PRIORITY + 2U)
#define TASK_LED2_PRIORITY         (tskIDLE_PRIORITY + 1U)

/* 一次性初始化任务：把裸机 main 中的初始化按阶段迁移到 RTOS。 */
static void TaskInit(void *pvParameters)
{
    (void)pvParameters;

    /* 第一阶段：先迁移不依赖复杂时序的初始化。 */
    Enroll_KEY_Init();
    Enroll_USART_Register();
    API_USART_Init(API_USART1, 115200U);

    /* 初始化完成后释放自身资源。 */
    vTaskDelete(0);
}

/* 任务1：LED1 每 1 秒翻转。 */
static void Task1(void *pvParameters)
{
    LED_Level_t level = LED_LOW;

    (void)pvParameters;

    for (;;)
    {
        level = (level == LED_LOW) ? LED_HIGH : LED_LOW;
        Enroll_LED_Control(LED1, level);
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

/* 任务2：LED2 每 0.5 秒翻转。 */
static void Task2(void *pvParameters)
{
    LED_Level_t level = LED_LOW;

    (void)pvParameters;

    for (;;)
    {
        level = (level == LED_LOW) ? LED_HIGH : LED_LOW;
        Enroll_LED_Control(LED2, level);
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

void ControlTask_RtosCreate(void)
{
    (void)xTaskCreate(TaskInit,
                      "init",
                      TASK_INIT_STACK_WORDS,
                      0,
                      TASK_INIT_PRIORITY,
                      0);

    (void)xTaskCreate(Task1,
                      "led1",
                      TASK_LED_STACK_WORDS,
                      0,
                      TASK_LED1_PRIORITY,
                      0);

    /* F103 板级只注册了 LED1，避免创建无效 LED2 任务。 */
#if (HW_LED_COUNT > 1U)
    (void)xTaskCreate(Task2,
                      "led2",
                      TASK_LED_STACK_WORDS,
                      0,
                      TASK_LED2_PRIORITY,
                      0);
#endif
}

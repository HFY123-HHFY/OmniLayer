#include "Control_Task_Rtos.h"

#include "FreeRTOS.h"
#include "task.h"

#include "Enroll.h"

#define APP_LED_TASK_STACK_WORDS   (128U)
#define APP_LED1_TASK_PRIORITY     (tskIDLE_PRIORITY + 2U)
#define APP_LED2_TASK_PRIORITY     (tskIDLE_PRIORITY + 1U)

static void ControlTask_Led1(void *param)
{
    LED_Level_t level = LED_LOW;

    (void)param;

    for (;;)
    {
        level = (level == LED_LOW) ? LED_HIGH : LED_LOW;
        Enroll_LED_Control(LED1, level);
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

static void ControlTask_Led2(void *param)
{
    LED_Level_t level = LED_LOW;

    (void)param;

    for (;;)
    {
        level = (level == LED_LOW) ? LED_HIGH : LED_LOW;
        Enroll_LED_Control(LED2, level);
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

void ControlTask_RtosCreate(void)
{
    (void)xTaskCreate(ControlTask_Led1,
                      "led1",
                      APP_LED_TASK_STACK_WORDS,
                      0,
                      APP_LED1_TASK_PRIORITY,
                      0);

    (void)xTaskCreate(ControlTask_Led2,
                      "led2",
                      APP_LED_TASK_STACK_WORDS,
                      0,
                      APP_LED2_TASK_PRIORITY,
                      0);
}

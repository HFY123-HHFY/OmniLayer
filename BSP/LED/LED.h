#ifndef __LED_H
#define __LED_H

#include <stdint.h>

/*
 * 如果这个头文件被 C++ 文件包含，就告诉编译器这些函数按 C 语言方式导出符号。
 * 这样可以避免 C++ 的名字改编（name mangling），保证 C 和 C++ 都能正常链接到这些函数。
 */
#ifdef __cplusplus
extern "C" {
#endif

/* LEDx 编号由 Enroll 层按实际板级资源进行注册。 */
typedef enum
{
	/* 第 1 个注册的 LED。 */
	LED1 = 0U,
	/* 第 2 个注册的 LED。 */
	LED2 = 1U,
	/* 第 3 个注册的 LED。 */
	LED3 = 2U
} LED_Id_t;

typedef enum
{
	/* 输出低电平。 */
	LED_LOW  = 0U,
	/* 输出高电平。 */
	LED_HIGH = 1U
} LED_Level_t;

/* LED 对接到底层 GPIO 的初始化函数指针。 */
typedef void (*LED_GPIO_InitFn)(void *port, uint32_t pin);
/* LED 对接到底层 GPIO 的写电平函数指针。 */
typedef void (*LED_GPIO_WriteFn)(void *port, uint32_t pin, uint8_t level);

typedef struct
{
	/* LED 逻辑编号：LED1/LED2/LED3... */
	LED_Id_t id;
	/* port 与 pin 由 Enroll 层填入具体 MCU 的 GPIO 资源。 */
	void *port;
	uint32_t pin;
	/* Core 层注入的 GPIO 初始化函数。 */
	LED_GPIO_InitFn gpioInit;
	/* Core 层注入的 GPIO 写电平函数。 */
	LED_GPIO_WriteFn gpioWrite;
} LED_Config_t;

/* Enroll 层调用：注册当前 MCU/板子的 LED 资源表。 */
void LED_Register(const LED_Config_t *configTable, uint8_t count);
/* BSP 层初始化：把所有已注册 LED 输出为同一个初始电平。 */
void LED_Init(LED_Level_t initLevel);
/* BSP 层控制：给指定 LED 写高/低电平。 */
void LED_Control(LED_Id_t id, LED_Level_t level);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */

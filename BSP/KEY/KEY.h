#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

/*
 * KEY 模块说明：
 * 1) 通过 Enroll 层注册板级按键资源；
 * 2) BSP 层只关心“哪个键对应哪个 GPIO”；
 * 3) 扫描函数 Key_Tick() 仍然保留，便于放到定时器中断里做轮询消抖。
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 按键逻辑编号，由 Enroll 层按板级资源注册。 */
typedef enum
{
	/* 第 1 个注册的按键。 */
	KEY1 = 0U
} KEY_Id_t;

/* 按键底层 GPIO 初始化函数指针。 */
typedef void (*KEY_GPIO_InitFn)(void *port, uint16_t pin);
/* 按键底层 GPIO 读取函数指针。 */
typedef uint8_t (*KEY_GPIO_ReadFn)(void *port, uint16_t pin);

/*
 * 按键板级配置项：
 * - id: 逻辑按键编号
 * - port/pin: 具体 GPIO 资源
 * - gpioInit: 底层输入初始化接口
 * - gpioRead: 底层电平读取接口
 */
typedef struct
{
	/* 按键逻辑编号。 */
	KEY_Id_t id;
	/* 由 Enroll 层注入的具体 GPIO 端口。 */
	void *port;
	/* 由 Enroll 层注入的具体 GPIO 引脚。 */
	uint16_t pin;
	/* 底层 GPIO 输入初始化函数。 */
	KEY_GPIO_InitFn gpioInit;
	/* 底层 GPIO 电平读取函数。 */
	KEY_GPIO_ReadFn gpioRead;
} KEY_Config_t;

/* Enroll 层调用：注册当前板子的按键资源表。 */
void KEY_Register(const KEY_Config_t *configTable, uint8_t count);
/* BSP 层初始化：把已注册的按键配置成可读取状态。 */
void KEY_Init(void);
/* 获取一次有效按键值。 */
uint8_t Key_GetNum(void);
/* 读取当前按键电平状态，按下返回 1，松开返回 0。 */
uint8_t Key_GetState(void);
/* 按键扫描函数，建议放到定时器中断里周期调用。 */
void Key_Tick(void);
/* 将扫描结果同步到全局 Key 变量，沿用你原来的使用方式。 */
void key_Get(void);

/* 当前最新按键值。 */
extern uint8_t Key;

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H */

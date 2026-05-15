#ifndef __API_EXTI_H
#define __API_EXTI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t API_EXTI_Id_t;

typedef enum
{
	API_EXTI_TRIGGER_RISING = 0x01U,
	API_EXTI_TRIGGER_FALLING = 0x02U
} API_EXTI_Trigger_t;

typedef struct
{
	API_EXTI_Id_t id;
	void *port;
	uint32_t pin;
} API_EXTI_Config_t;


typedef void (*API_EXTI_IrqHandler_t)(API_EXTI_Id_t id, void *userData);

typedef struct API_EXTI_IrqHandlerNode {
	API_EXTI_IrqHandler_t handler;
	void *userData;
	struct API_EXTI_IrqHandlerNode *next;
} API_EXTI_IrqHandlerNode_t;

void API_EXTI_Register(const API_EXTI_Config_t *configTable, uint8_t count);
// 多路注册：同一 id 可多次注册不同回调
int API_EXTI_AddIrqHandler(API_EXTI_Id_t id, API_EXTI_IrqHandler_t handler, void *userData);
// 注销指定回调
int API_EXTI_RemoveIrqHandler(API_EXTI_Id_t id, API_EXTI_IrqHandler_t handler, void *userData);
// 清空所有回调
void API_EXTI_ClearIrqHandlers(API_EXTI_Id_t id);

/* 初始化 EXTI 资源：按注册表中的 id 查找端口/引脚，配置触发沿和优先级。 */
void API_EXTI_Init(API_EXTI_Id_t id, API_EXTI_Trigger_t trigger,
	uint8_t preemptPriority, uint8_t subPriority);

/* 平台 IRQ 入口统一转发到 API 层。 */
void API_EXTI_HandleIrqByLine(uint8_t lineIndex);
void API_EXTI_HandleIrqByLineGroup(uint8_t startLine, uint8_t endLine);
void API_EXTI_HandleIrqByPort(void *port);

/*
 * 用法示例：
 * // 1. 定义注册表
 * static const API_EXTI_Config_t extiTable[] = {
 *     { .id = 0, .port = GPIOA, .pin = GPIO_PIN_0 },
 *     { .id = 1, .port = GPIOB, .pin = GPIO_PIN_1 },
 * };
 * API_EXTI_Register(extiTable, sizeof(extiTable)/sizeof(extiTable[0]));
 * // 2. 注册回调
 * API_EXTI_AddIrqHandler(0, my_exti0_callback, myUserPtr0);
 * API_EXTI_AddIrqHandler(0, another_callback, NULL);
 * API_EXTI_AddIrqHandler(1, my_exti1_callback, myUserPtr1);
 * // 3. 回调原型
 * void my_exti0_callback(API_EXTI_Id_t id, void *userData) { ... }
 */

#ifdef __cplusplus
}
#endif

#endif /* __API_EXTI_H */

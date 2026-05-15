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

typedef void (*API_EXTI_IrqHandler_t)(API_EXTI_Id_t id);

void API_EXTI_Register(const API_EXTI_Config_t *configTable, uint8_t count);
void API_EXTI_RegisterIrqHandler(API_EXTI_Id_t id, API_EXTI_IrqHandler_t handler);

/* 初始化 EXTI 资源：按注册表中的 id 查找端口/引脚，配置触发沿和优先级。 */
void API_EXTI_Init(API_EXTI_Id_t id, API_EXTI_Trigger_t trigger,
	uint8_t preemptPriority, uint8_t subPriority);

/* 平台 IRQ 入口统一转发到 API 层。 */
void API_EXTI_HandleIrqByLine(uint8_t lineIndex);
void API_EXTI_HandleIrqByLineGroup(uint8_t startLine, uint8_t endLine);
void API_EXTI_HandleIrqByPort(void *port);

#ifdef __cplusplus
}
#endif

#endif /* __API_EXTI_H */

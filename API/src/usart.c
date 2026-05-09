#include "usart.h"
#include "gpio.h"
#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#include "G3507_hw_config.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_uart_main.h"
#endif

static const API_USART_Config_t *s_usartTable;
static uint8_t s_usartCount;
static API_USART_IrqHandler_t s_usartIrqHandlers[API_USART4 + 1U];

#ifndef API_USART_CR1_TXEIE
#define API_USART_CR1_TXEIE (1UL << 7)
#endif

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
/* 配置 F103 的 TX 引脚为复用推挽输出。 */
static void API_USART_ConfigTxPin(void *port, uint16_t pin)
{
	F103_GPIO_Regs_t *gpioPort;
	uint32_t pinIndex;
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	F103_GPIO_EnablePortClock(port);
	pinIndex = F103_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	shift = (pinIndex & 0x7U) * 4U;
	if (pinIndex < 8U)
	{
		gpioPort->CRL &= ~(0xFUL << shift);
		gpioPort->CRL |= (0xBUL << shift);
	}
	else
	{
		gpioPort->CRH &= ~(0xFUL << shift);
		gpioPort->CRH |= (0xBUL << shift);
	}
}

/* 配置 F103 的 RX 引脚为浮空输入。 */
static void API_USART_ConfigRxPin(void *port, uint16_t pin)
{
	F103_GPIO_Regs_t *gpioPort;
	uint32_t pinIndex;
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F103_GPIO_Regs_t *)port;
	F103_GPIO_EnablePortClock(port);
	pinIndex = F103_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	shift = (pinIndex & 0x7U) * 4U;
	if (pinIndex < 8U)
	{
		gpioPort->CRL &= ~(0xFUL << shift);
		gpioPort->CRL |= (0x4UL << shift);
	}
	else
	{
		gpioPort->CRH &= ~(0xFUL << shift);
		gpioPort->CRH |= (0x4UL << shift);
	}
}

#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
/* USART1/2/3/4 对应的复用功能号。 */
static uint8_t API_USART_GetAfNum(API_USART_Id_t id)
{
	return (id == API_USART4) ? 8U : 7U;
}

/* 配置 F407 的 TX/RX 引脚为复用模式。 */
static void API_USART_ConfigAfPin(void *port, uint16_t pin, uint8_t af)
{
	F407_GPIO_Regs_t *gpioPort;
	uint32_t pinIndex;
	uint32_t shift;

	if ((port == 0) || (pin == 0U))
	{
		return;
	}

	gpioPort = (F407_GPIO_Regs_t *)port;
	F407_GPIO_EnablePortClock(port);
	pinIndex = F407_GPIO_PinIndex(pin);
	if (pinIndex > 15U)
	{
		return;
	}

	shift = pinIndex * 2U;
	gpioPort->MODER &= ~(0x3UL << shift);
	gpioPort->MODER |= (0x2UL << shift);
	gpioPort->OTYPER &= ~(1UL << pinIndex);
	gpioPort->OSPEEDR &= ~(0x3UL << shift);
	gpioPort->OSPEEDR |= (0x2UL << shift);
	gpioPort->PUPDR &= ~(0x3UL << shift);

	if (pinIndex < 8U)
	{
		shift = pinIndex * 4U;
		gpioPort->AFRL &= ~(0xFUL << shift);
		gpioPort->AFRL |= ((uint32_t)af << shift);
	}
	else
	{
		shift = (pinIndex - 8U) * 4U;
		gpioPort->AFRH &= ~(0xFUL << shift);
		gpioPort->AFRH |= ((uint32_t)af << shift);
	}
}

#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
static uint8_t API_USART_GetG3507Pinmux(
	API_USART_Id_t id, uint8_t isTx, uint32_t *iomux, uint32_t *func)
{
	if ((iomux == 0) || (func == 0))
	{
		return 0U;
	}

	switch (id)
	{
	case API_USART1:
		if (isTx != 0U)
		{
			*iomux = G3507_USART0_TX_IOMUX;
			*func = G3507_USART0_TX_FUNC;
		}
		else
		{
			*iomux = G3507_USART0_RX_IOMUX;
			*func = G3507_USART0_RX_FUNC;
		}
		return 1U;
	default:
		break;
	}

	return 0U;
}

static void API_USART_ConfigTxPin(API_USART_Id_t id, void *port, uint16_t pin)
{
	uint32_t iomux;
	uint32_t func;

	(void)port;
	(void)pin;
	if (API_USART_GetG3507Pinmux(id, 1U, &iomux, &func) == 0U)
	{
		return;
	}
	DL_GPIO_initPeripheralOutputFunction(iomux, func);
}

static void API_USART_ConfigRxPin(API_USART_Id_t id, void *port, uint16_t pin)
{
	uint32_t iomux;
	uint32_t func;

	(void)port;
	(void)pin;
	if (API_USART_GetG3507Pinmux(id, 0U, &iomux, &func) == 0U)
	{
		return;
	}
	DL_GPIO_initPeripheralInputFunction(iomux, func);
}
#else
static void API_USART_ConfigTxPin(void *port, uint16_t pin)
{
	(void)port;
	(void)pin;
}

static void API_USART_ConfigRxPin(void *port, uint16_t pin)
{
	(void)port;
	(void)pin;
}

static uint8_t API_USART_GetAfNum(API_USART_Id_t id)
{
	(void)id;
	return 0U;
}

static void API_USART_ConfigAfPin(void *port, uint16_t pin, uint8_t af)
{
	(void)port;
	(void)pin;
	(void)af;
}
#endif

/*
 * 串口底层初始化：
 * 这层只负责注册、引脚复用和调用 Core 初始化，不处理接收中断包装。
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
static void API_USART_CoreInit(uint8_t coreId, uint32_t baudRate)
{
	F103_USART_Init(coreId, baudRate);
}

static void API_USART_CoreWriteByte(uint8_t coreId, uint8_t data)
{
	F103_USART_WriteByte(coreId, data);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
static void API_USART_CoreInit(uint8_t coreId, uint32_t baudRate)
{
	F407_USART_Init(coreId, baudRate);
}

static void API_USART_CoreWriteByte(uint8_t coreId, uint8_t data)
{
	F407_USART_WriteByte(coreId, data);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
static void API_USART_CoreInit(uint8_t coreId, uint32_t baudRate)
{
	G3507_USART_Init(coreId, baudRate);
}

static void API_USART_CoreWriteByte(uint8_t coreId, uint8_t data)
{
	G3507_USART_WriteByte(coreId, data);
}
#else
static void API_USART_CoreInit(uint8_t coreId, uint32_t baudRate)
{
	(void)coreId;
	(void)baudRate;
}

static void API_USART_CoreWriteByte(uint8_t coreId, uint8_t data)
{
	(void)coreId;
	(void)data;
}
#endif

/* 在注册表中查找指定串口。 */
static const API_USART_Config_t *API_USART_FindConfig(API_USART_Id_t id)
{
	uint8_t index;

	if ((s_usartTable == 0) || (s_usartCount == 0U))
	{
		return 0;
	}

	for (index = 0U; index < s_usartCount; ++index)
	{
		if (s_usartTable[index].id == id)
		{
			return &s_usartTable[index];
		}
	}

	return 0;
}

/* 在注册表中按 Core USART 索引查找配置。 */
static const API_USART_Config_t *API_USART_FindConfigByCoreId(uint8_t coreId)
{
	uint8_t index;

	if ((s_usartTable == 0) || (s_usartCount == 0U))
	{
		return 0;
	}

	for (index = 0U; index < s_usartCount; ++index)
	{
		if (s_usartTable[index].coreId == coreId)
		{
			return &s_usartTable[index];
		}
	}

	return 0;
}

/* 注册板级串口资源映射表。 */
void API_USART_Register(const API_USART_Config_t *configTable, uint8_t count)
{
	s_usartTable = configTable;
	s_usartCount = count;
}

void API_USART_RegisterIrqHandler(API_USART_Id_t id, API_USART_IrqHandler_t handler)
{
	if ((id < API_USART1) || (id > API_USART4))
	{
		return;
	}

	s_usartIrqHandlers[id] = handler;
}

/* 串口初始化入口：id 选串口，baudRate 配置波特率。 */
void API_USART_Init(API_USART_Id_t id, uint32_t baudRate)
{
	const API_USART_Config_t *config;

	config = API_USART_FindConfig(id);
	if (config == 0)
	{
		return;
	}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	if ((config->txPort != 0) && (config->txPin != 0U))
	{
		API_USART_ConfigTxPin(config->txPort, config->txPin);
	}

	if ((config->rxPort != 0) && (config->rxPin != 0U))
	{
		API_USART_ConfigRxPin(config->rxPort, config->rxPin);
	}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	if ((config->txPort != 0) && (config->txPin != 0U))
	{
		API_USART_ConfigAfPin(config->txPort, config->txPin, API_USART_GetAfNum(id));
	}

	if ((config->rxPort != 0) && (config->rxPin != 0U))
	{
		API_USART_ConfigAfPin(config->rxPort, config->rxPin, API_USART_GetAfNum(id));
	}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	if ((config->txPort != 0) && (config->txPin != 0U))
	{
		API_USART_ConfigTxPin(id, config->txPort, config->txPin);
	}

	if ((config->rxPort != 0) && (config->rxPin != 0U))
	{
		API_USART_ConfigRxPin(id, config->rxPort, config->rxPin);
	}
#endif

	if (baudRate == 0U)
	{
		return;
	}

	API_USART_CoreInit(config->coreId, baudRate);
}

/* 串口发送 1 字节。 */
void API_USART_WriteByte(API_USART_Id_t id, uint8_t data)
{
	const API_USART_Config_t *config;

	config = API_USART_FindConfig(id);
	if (config == 0)
	{
		return;
	}

	API_USART_CoreWriteByte(config->coreId, data);
}

void API_USART_HandleIrqByCoreId(uint8_t coreId)
{
	const API_USART_Config_t *config;
	API_USART_IrqHandler_t handler;

	config = API_USART_FindConfigByCoreId(coreId);
	if (config == 0)
	{
		return;
	}

	if ((config->id < API_USART1) || (config->id > API_USART4))
	{
		return;
	}

	handler = s_usartIrqHandlers[config->id];
	if (handler == 0)
	{
		return;
	}

	handler(config->id);
}

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
void USART1_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_USART1);
}

void USART2_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_USART2);
}

void USART3_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_USART3);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
void USART1_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_USART1);
}

void USART2_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_USART2);
}

void USART3_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_USART3);
}

void UART4_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_USART4);
}
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
void UART0_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_UART0);
}

void UART1_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_UART1);
}

void UART2_IRQHandler(void)
{
	API_USART_HandleIrqByCoreId(API_USART_CORE_UART2);
}
#endif

#include "G3507_usart.h"
#include "IrqPriority.h"

#include "G3507_sys.h"

#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_uart_main.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

typedef struct
{
	UART_Regs *regs;
	IRQn_Type irq;
} G3507_USART_Map_t;

static G3507_USART_Map_t G3507_USART_GetMap(uint8_t usartId)
{
	G3507_USART_Map_t map;

	map.regs = 0;
	map.irq = NonMaskableInt_IRQn;

	switch (usartId)
	{
	case 0U:
		map.regs = UART0;
		map.irq = UART0_INT_IRQn;
		break;
	case 1U:
		map.regs = UART1;
		map.irq = UART1_INT_IRQn;
		break;
	case 2U:
		map.regs = UART2;
		map.irq = UART2_INT_IRQn;
		break;
	case 3U:
		map.regs = UART3;
		map.irq = UART3_INT_IRQn;
		break;
	default:
		break;
	}

	return map;
}

void G3507_USART_Init(uint8_t usartId, uint32_t baudRate)
{
	G3507_USART_Map_t map;
	DL_UART_Main_ClockConfig clockConfig;
	DL_UART_Main_Config uartConfig;
	uint32_t busClkHz;

	map = G3507_USART_GetMap(usartId);
	if ((map.regs == 0) || (baudRate == 0UL))
	{
		return;
	}

	if (!DL_UART_Main_isPowerEnabled(map.regs))
	{
		DL_UART_Main_reset(map.regs);
		DL_UART_Main_enablePower(map.regs);
		while (!DL_UART_Main_isPowerEnabled(map.regs))
		{
		}
	}

	DL_UART_Main_disable(map.regs);

	clockConfig.clockSel = DL_UART_MAIN_CLOCK_BUSCLK;
	clockConfig.divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1;
	DL_UART_Main_setClockConfig(map.regs, &clockConfig);

	uartConfig.mode = DL_UART_MAIN_MODE_NORMAL;
	uartConfig.direction = DL_UART_MAIN_DIRECTION_TX_RX;
	uartConfig.flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE;
	uartConfig.parity = DL_UART_MAIN_PARITY_NONE;
	uartConfig.wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS;
	uartConfig.stopBits = DL_UART_MAIN_STOP_BITS_ONE;
	DL_UART_Main_init(map.regs, &uartConfig);

	busClkHz = G3507_SYS_GetBusClkHz();
	if (busClkHz == 0UL)
	{
		busClkHz = 32000000UL;
	}
	DL_UART_Main_configBaudRate(map.regs, busClkHz, baudRate);

	DL_UART_Main_enableInterrupt(map.regs, DL_UART_MAIN_INTERRUPT_RX);
	DL_UART_Main_enable(map.regs);
	NVIC_SetPriority(map.irq, IRQ_PRIO_USART);
	NVIC_EnableIRQ(map.irq);
}

void G3507_USART_WriteByte(uint8_t usartId, uint8_t data)
{
	G3507_USART_Map_t map;

	map = G3507_USART_GetMap(usartId);
	if (map.regs == 0)
	{
		return;
	}

	DL_UART_Main_transmitDataBlocking(map.regs, (uint8_t)data);
}

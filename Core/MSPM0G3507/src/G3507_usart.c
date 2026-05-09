#include "G3507_usart.h"

#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/m0p/dl_sysctl.h"
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
	default:
		break;
	}

	return map;
}

/* 与 Delay 层保持一致，按当前时钟树估算 MCLK。 */
static uint32_t G3507_USART_GetMclkHz(void)
{
	uint32_t sourceHz;
	uint32_t divider;

	if (DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_LFCLK)
	{
		return 32768UL;
	}

	if (DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_HSCLK)
	{
		return 32000000UL;
	}

	if (DL_SYSCTL_getCurrentSYSOSCFreq() == DL_SYSCTL_SYSOSC_FREQ_4M)
	{
		sourceHz = 4000000UL;
	}
	else
	{
		sourceHz = 32000000UL;
	}

	divider = (uint32_t)DL_SYSCTL_getMCLKDivider();
	if (divider == (uint32_t)DL_SYSCTL_MCLK_DIVIDER_DISABLE)
	{
		divider = 1UL;
	}
	else
	{
		divider += 1UL;
	}

	if (divider == 0UL)
	{
		divider = 1UL;
	}

	return (sourceHz / divider);
}

void G3507_USART_Init(uint8_t usartId, uint32_t baudRate)
{
	G3507_USART_Map_t map;
	DL_UART_Main_ClockConfig clockConfig;
	DL_UART_Main_Config uartConfig;
	uint32_t mclkHz;

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

	mclkHz = G3507_USART_GetMclkHz();
	DL_UART_Main_configBaudRate(map.regs, mclkHz, baudRate);

	DL_UART_Main_enableInterrupt(map.regs, DL_UART_MAIN_INTERRUPT_RX);
	DL_UART_Main_enable(map.regs);
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

	while (!DL_UART_Main_transmitDataCheck(map.regs, (uint8_t)data))
	{
	}

	DL_UART_Main_transmitData(map.regs, (uint16_t)data);
}

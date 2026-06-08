#include "G3507_tim.h"
#include "IrqPriority.h"

#include "G3507_sys.h"

#include "ti/driverlib/dl_timerg.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

#define G3507_TIM_TICK_HZ             (1000000UL)
#define G3507_TIM_ISR_MASK            (DL_TIMERG_INTERRUPT_ZERO_EVENT)

typedef struct
{
	GPTIMER_Regs *regs;
	IRQn_Type irq;
} G3507_TIM_Map_t;

static G3507_TIM_Map_t G3507_TIM_GetMap(uint8_t timId)
{
	G3507_TIM_Map_t map;

	map.regs = 0;
	map.irq = NonMaskableInt_IRQn;

	switch (timId)
	{
	case 0U:
		map.regs = TIMG0;
		map.irq = TIMG0_INT_IRQn;
		break;
	case 1U:
		map.regs = TIMG6;
		map.irq = TIMG6_INT_IRQn;
		break;
	case 2U:
		map.regs = TIMA0;
		map.irq = TIMA0_INT_IRQn;
		break;
	case 3U:
		map.regs = TIMA1;
		map.irq = TIMA1_INT_IRQn;
		break;
	case 4U:
		map.regs = TIMG7;
		map.irq = TIMG7_INT_IRQn;
		break;
	case 5U:
		map.regs = TIMG8;
		map.irq = TIMG8_INT_IRQn;
		break;
	case 6U:
		map.regs = TIMG12;
		map.irq = TIMG12_INT_IRQn;
		break;
	default:
		break;
	}

	return map;
}

void G3507_TIM_PeriodicInit(uint8_t timId, uint32_t periodMs)
{
	G3507_TIM_Map_t map;
	DL_TimerG_ClockConfig clockConfig;
	DL_TimerG_TimerConfig timerConfig;
	uint32_t busClkHz;
	uint32_t periodTicks;

	if (periodMs == 0U)
	{
		return;
	}

	map = G3507_TIM_GetMap(timId);
	if (map.regs == 0)
	{
		return;
	}

	if (!DL_TimerG_isPowerEnabled(map.regs))
	{
		DL_TimerG_reset(map.regs);
		DL_TimerG_enablePower(map.regs);
		while (!DL_TimerG_isPowerEnabled(map.regs))
		{
		}
		/* TI SysConfig: 16-cycle delay after power-up for register stability */
		__ISB();
		__ISB();
	}

	/*
	 * Clock tree (TI SysConfig style):
	 *   timerClk = BUSCLK / divideRatio / (prescale + 1)
	 *   DIVIDE_8 -> 10MHz, then CPS -> 1MHz tick.
	 */
	clockConfig.clockSel     = DL_TIMER_CLOCK_BUSCLK;
	clockConfig.divideRatio  = DL_TIMER_CLOCK_DIVIDE_8;

	/*
	 * MSPM0G3507 dual power-domain clock difference:
	 * - PD1 (TIMG0, TIMA0, TIMA1): BUSCLK = MCLK / ULPCLK_div (40MHz)
	 * - PD0 (TIMG6, TIMG7, TIMG8, TIMG12): direct MCLK (80MHz)
	 * Must use the actual clock frequency for prescaler calculation.
	 */
	if (timId == 1U || timId == 4U || timId == 5U || timId == 6U)
	{
		busClkHz = G3507_SYS_GetMclkHz(); /* PD0: direct MCLK */
	}
	else
	{
		busClkHz = G3507_SYS_GetBusClkHz(); /* PD1: via ULPCLK divider */
	}
	if (busClkHz == 0UL)
	{
		busClkHz = 80000000UL;
	}

	{
		uint32_t clkAfterDiv;
		uint32_t prescale;

		clkAfterDiv = busClkHz / 8UL;
		prescale    = clkAfterDiv / G3507_TIM_TICK_HZ;
		if (prescale == 0UL) { prescale = 1UL; }
		if (prescale > 256UL) { prescale = 256UL; }

		clockConfig.prescale = (uint8_t)(prescale - 1UL);
		periodTicks = periodMs * (G3507_TIM_TICK_HZ / 1000UL);
	}

	DL_TimerG_setClockConfig(map.regs, &clockConfig);

	if (periodTicks == 0UL)
	{
		periodTicks = 1UL;
	}

	/*
	 * TI SysConfig style: startTimer=START inside initTimerMode,
	 * then enableClock to start counting. No separate startCounter.
	 */
	timerConfig.timerMode    = DL_TIMER_TIMER_MODE_PERIODIC;
	timerConfig.period       = periodTicks - 1UL;
	timerConfig.startTimer   = DL_TIMER_START;
	timerConfig.genIntermInt = DL_TIMER_INTERM_INT_DISABLED;
	timerConfig.counterVal   = 0UL;
	DL_TimerG_initTimerMode(map.regs, &timerConfig);

	/* Enable counter clock AFTER clock config and timer mode are set */
	DL_TimerG_enableClock(map.regs);

	DL_TimerG_clearInterruptStatus(map.regs, G3507_TIM_ISR_MASK);
	DL_TimerG_enableInterrupt(map.regs, G3507_TIM_ISR_MASK);
	NVIC_ClearPendingIRQ(map.irq);
	NVIC_SetPriority(map.irq, IRQ_PRIO_TIM_CTRL);
	NVIC_EnableIRQ(map.irq);
}

uint8_t G3507_TIM_CheckAndClearUpdateIrq(uint8_t timId)
{
	G3507_TIM_Map_t map;

	map = G3507_TIM_GetMap(timId);
	if (map.regs == 0)
	{
		return 0U;
	}

	if ((DL_TimerG_getRawInterruptStatus(map.regs, G3507_TIM_ISR_MASK) & G3507_TIM_ISR_MASK) == 0U)
	{
		return 0U;
	}

	DL_TimerG_clearInterruptStatus(map.regs, G3507_TIM_ISR_MASK);
	return 1U;
}

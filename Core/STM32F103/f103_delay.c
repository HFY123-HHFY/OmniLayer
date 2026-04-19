#include "delay.h"
#include "stm32f10x.h"

/* 由 CMSIS 系统文件维护：表示当前 HCLK 频率（单位 Hz）。 */
extern uint32_t SystemCoreClock;

/*
 * 用 SysTick 做一次阻塞延时（ticks 最大 0xFFFFFF）。
 * SysTick->LOAD 存的是重装值，因此写入 ticks-1。
 */
static void F103_SysTickDelayTicks(uint32_t ticks)
{
	if (ticks == 0U)
	{
		return;
	}

	SysTick->LOAD = ticks - 1U;
	SysTick->VAL = 0U;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

	while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U)
	{
		/* 等待倒计时结束。 */
	}

	SysTick->CTRL = 0U;
	SysTick->VAL = 0U;
}

/* 计算每微秒对应的 SysTick 计数（至少为 1）。 */
static uint32_t F103_TicksPerUs(void)
{
	uint32_t ticksPerUs;

	ticksPerUs = SystemCoreClock / 1000000U;
	if (ticksPerUs == 0U)
	{
		ticksPerUs = 1U;
	}

	return ticksPerUs;
}

void Delay_us(uint32_t us)
{
	uint32_t ticksPerUs;
	uint32_t maxUsPerShot;

	if (us == 0U)
	{
		return;
	}

	ticksPerUs = F103_TicksPerUs();
	maxUsPerShot = 0xFFFFFFU / ticksPerUs;
	if (maxUsPerShot == 0U)
	{
		maxUsPerShot = 1U;
	}

	while (us > 0U)
	{
		uint32_t thisUs;
		uint32_t thisTicks;

		thisUs = (us > maxUsPerShot) ? maxUsPerShot : us;
		thisTicks = thisUs * ticksPerUs;

		F103_SysTickDelayTicks(thisTicks);
		us -= thisUs;
	}
}

void Delay_ms(uint32_t ms)
{
	while (ms > 0U)
	{
		Delay_us(1000U);
		--ms;
	}
}

void Delay_s(uint32_t s)
{
	while (s > 0U)
	{
		Delay_ms(1000U);
		--s;
	}
}

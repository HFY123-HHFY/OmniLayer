#include "delay.h"

#include "stm32f4xx.h"

/* 由 CMSIS 系统文件维护：表示当前 HCLK 频率（单位 Hz）。 */
extern uint32_t SystemCoreClock;

/*
 * F407 参考实现采用 SysTick 时钟源 HCLK/8。
 * facUs: 1us 对应的 SysTick 计数；facMs: 1ms 对应计数。
 */
static void F407_GetDelayFactor(uint32_t *facUs, uint32_t *facMs)
{
	uint32_t sysclkDiv8;

	sysclkDiv8 = SystemCoreClock / 8U;
	*facUs = sysclkDiv8 / 1000000U;
	if (*facUs == 0U)
	{
		*facUs = 1U;
	}
	*facMs = (*facUs) * 1000U;
}

/* 单次毫秒延时（受 24bit SysTick LOAD 限制）。 */
static void F407_DelayXms(uint32_t ms)
{
	uint32_t facUs;
	uint32_t facMs;
	uint32_t temp;

	if (ms == 0U)
	{
		return;
	}

	F407_GetDelayFactor(&facUs, &facMs);
	SysTick->LOAD = ms * facMs;
	SysTick->VAL = 0U;
	SysTick->CTRL = 0x01U;
	do
	{
		temp = SysTick->CTRL;
	} while (((temp & 0x01U) != 0U) && ((temp & (1U << 16U)) == 0U));
	SysTick->CTRL = 0U;
	SysTick->VAL = 0U;
}

void Delay_us(uint32_t us)
{
	uint32_t facUs;
	uint32_t facMs;
	uint32_t temp;
	uint32_t maxUsPerShot;

	if (us == 0U)
	{
		return;
	}

	F407_GetDelayFactor(&facUs, &facMs);
	maxUsPerShot = 0xFFFFFFU / facUs;
	if (maxUsPerShot == 0U)
	{
		maxUsPerShot = 1U;
	}

	while (us > 0U)
	{
		uint32_t thisUs;

		thisUs = (us > maxUsPerShot) ? maxUsPerShot : us;
		SysTick->LOAD = thisUs * facUs;
		SysTick->VAL = 0U;
		SysTick->CTRL = 0x01U;
		do
		{
			temp = SysTick->CTRL;
		} while (((temp & 0x01U) != 0U) && ((temp & (1U << 16U)) == 0U));
		SysTick->CTRL = 0U;
		SysTick->VAL = 0U;
		us -= thisUs;
	}
}

void Delay_ms(uint32_t ms)
{
	/* 与常见 F407 参考工程一致：每段 540ms，兼顾超频余量。 */
	while (ms > 540U)
	{
		F407_DelayXms(540U);
		ms -= 540U;
	}

	if (ms > 0U)
	{
		F407_DelayXms(ms);
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

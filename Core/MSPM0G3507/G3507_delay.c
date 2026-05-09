#include "Delay.h"
#include "ti/driverlib/m0p/dl_sysctl.h"

/* SysTick 24-bit 计数器最大装载值。 */
#define G3507_SYSTICK_MAX_RELOAD (0x00FFFFFFUL)

/* 延时模块内部缓存：当前 MCLK 频率与 SysTick 初始化状态。 */
static uint32_t s_mclkHz = 4000000UL;
static uint8_t s_systickReady = 0U;

/* 根据当前时钟树估算 MCLK 频率。 */
static uint32_t G3507_GetMclkHz(void)
{
	uint32_t sourceHz;
	uint32_t divider;

	if (DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_LFCLK)
	{
		/* LFCLK 为 32.768kHz。 */
		return 32768UL;
	}

	if (DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_HSCLK)
	{
		/* 目前工程未切 HSCLK，保守按 32MHz 处理。 */
		return 32000000UL;
	}

	/* SYSOSC 源：4MHz 或 BASE(典型 32MHz)。 */
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
		/* 枚举值 1..15 分别表示 /2../16。 */
		divider += 1UL;
	}

	if (divider == 0UL)
	{
		divider = 1UL;
	}

	return (sourceHz / divider);
}

/* 初始化 SysTick 为自由运行计数器。 */
static void G3507_DelayInit(void)
{
	if (s_systickReady != 0U)
	{
		return;
	}

	s_mclkHz = G3507_GetMclkHz();
	if (s_mclkHz == 0UL)
	{
		s_mclkHz = 4000000UL;
	}

	SysTick->LOAD = G3507_SYSTICK_MAX_RELOAD;
	SysTick->VAL = 0UL;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
	s_systickReady = 1U;
}

/* 按 CPU 周期阻塞等待。 */
static void G3507_DelayCycles(uint32_t cycles)
{
	uint32_t prev;
	uint32_t now;
	uint32_t elapsed;

	if (cycles == 0UL)
	{
		return;
	}

	prev = SysTick->VAL;
	while (cycles != 0UL)
	{
		now = SysTick->VAL;
		if (prev >= now)
		{
			elapsed = prev - now;
		}
		else
		{
			elapsed = prev + (G3507_SYSTICK_MAX_RELOAD + 1UL) - now;
		}

		if (elapsed >= cycles)
		{
			cycles = 0UL;
		}
		else
		{
			cycles -= elapsed;
			prev = now;
		}
	}
}

/* 微秒延时。 */
void Delay_us(uint32_t us)
{
	uint64_t cycles;

	G3507_DelayInit();

	cycles = ((uint64_t)s_mclkHz * (uint64_t)us) / 1000000ULL;
	if (cycles == 0ULL)
	{
		cycles = 1ULL;
	}

	while (cycles > 0ULL)
	{
		uint32_t chunk = (cycles > (uint64_t)G3507_SYSTICK_MAX_RELOAD) ?
			G3507_SYSTICK_MAX_RELOAD : (uint32_t)cycles;
		G3507_DelayCycles(chunk);
		cycles -= (uint64_t)chunk;
	}
}

/* 毫秒延时。 */
void Delay_ms(uint32_t ms)
{
	volatile uint32_t i;

	for (i = 0U; i < ms; ++i)
	{
		Delay_us(1000U);
	}
}

/* 秒延时。 */
void Delay_s(uint32_t s)
{
	volatile uint32_t i;

	for (i = 0U; i < s; ++i)
	{
		Delay_ms(1000U);
	}
}

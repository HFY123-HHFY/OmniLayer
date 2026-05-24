#include "sys.h"

#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "ti/driverlib/dl_common.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti/driverlib/m0p/dl_sysctl.h"

/* G3507 时钟策略：默认启用 SYSPLL 提升到 80MHz。 */
#ifndef G3507_ENABLE_PLL80
#define G3507_ENABLE_PLL80 (1U)
#endif

#define G3507_STARTUP_SETTLE_CYCLES_FAST   (80000UL)
#define G3507_STARTUP_SETTLE_CYCLES_COLD   (400000UL)
#define G3507_POWER_GOOD_TIMEOUT           (800000UL)
#define G3507_PLL_LOCK_TIMEOUT             (1200000UL)
#define G3507_HSCLK_SWITCH_TIMEOUT         (600000UL)

static void G3507_BusyWaitCycles(volatile uint32_t cycles)
{
	while (cycles > 0UL)
	{
		__NOP();
		cycles--;
	}
}

static void G3507_WaitPowerGood(void)
{
	uint32_t timeout;
	uint32_t status;

	timeout = G3507_POWER_GOOD_TIMEOUT;
	while (timeout > 0UL)
	{
		status = DL_SYSCTL_getStatus();
		if ((status & (DL_SYSCTL_STATUS_PMU_IFREF_GOOD | DL_SYSCTL_STATUS_VBOOST_GOOD)) ==
			(DL_SYSCTL_STATUS_PMU_IFREF_GOOD | DL_SYSCTL_STATUS_VBOOST_GOOD))
		{
			break;
		}
		timeout--;
	}
}

static uint8_t G3507_WaitClockStatus(uint32_t mask, uint32_t expectValue, uint32_t timeout)
{
	while (timeout > 0UL)
	{
		if ((DL_SYSCTL_getClockStatus() & mask) == expectValue)
		{
			return 1U;
		}
		timeout--;
	}

	return 0U;
}

static uint8_t G3507_ConfigSysPll80WithTimeout(void)
{
	DL_SYSCTL_SYSPLLConfig pllConfig;
	uint32_t ctlTemp;

	pllConfig.rDivClk2x = 3U;
	pllConfig.rDivClk1 = 0U;
	pllConfig.rDivClk0 = 0U;
	pllConfig.enableCLK2x = DL_SYSCTL_SYSPLL_CLK2X_ENABLE;
	pllConfig.enableCLK1 = DL_SYSCTL_SYSPLL_CLK1_DISABLE;
	pllConfig.enableCLK0 = DL_SYSCTL_SYSPLL_CLK0_DISABLE;
	pllConfig.sysPLLMCLK = DL_SYSCTL_SYSPLL_MCLK_CLK2X;
	pllConfig.sysPLLRef = DL_SYSCTL_SYSPLL_REF_SYSOSC;
	pllConfig.qDiv = 4U;
	pllConfig.pDiv = DL_SYSCTL_SYSPLL_PDIV_1;
	pllConfig.inputFreq = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ;

	DL_SYSCTL_disableSYSPLL();
	if (G3507_WaitClockStatus(SYSCTL_CLKSTATUS_SYSPLLOFF_MASK,
		DL_SYSCTL_CLK_STATUS_SYSPLL_OFF,
		G3507_PLL_LOCK_TIMEOUT) == 0U)
	{
		return 0U;
	}

	DL_Common_updateReg(&SYSCTL->SOCLOCK.SYSPLLCFG0,
		(uint32_t)pllConfig.sysPLLRef,
		SYSCTL_SYSPLLCFG0_SYSPLLREF_MASK);

	DL_Common_updateReg(&SYSCTL->SOCLOCK.SYSPLLCFG1,
		(uint32_t)pllConfig.pDiv,
		SYSCTL_SYSPLLCFG1_PDIV_MASK);

	ctlTemp = DL_CORE_getInstructionConfig();
	DL_CORE_configInstruction(DL_CORE_PREFETCH_ENABLED,
		DL_CORE_CACHE_DISABLED,
		DL_CORE_LITERAL_CACHE_ENABLED);

	SYSCTL->SOCLOCK.SYSPLLPARAM0 = *(volatile uint32_t *)((uint32_t)pllConfig.inputFreq);
	SYSCTL->SOCLOCK.SYSPLLPARAM1 = *(volatile uint32_t *)((uint32_t)pllConfig.inputFreq + 4UL);

	CPUSS->CTL = ctlTemp;

	DL_Common_updateReg(&SYSCTL->SOCLOCK.SYSPLLCFG1,
		((pllConfig.qDiv << SYSCTL_SYSPLLCFG1_QDIV_OFS) & SYSCTL_SYSPLLCFG1_QDIV_MASK),
		SYSCTL_SYSPLLCFG1_QDIV_MASK);

	DL_Common_updateReg(&SYSCTL->SOCLOCK.SYSPLLCFG0,
		(((pllConfig.rDivClk2x << SYSCTL_SYSPLLCFG0_RDIVCLK2X_OFS) & SYSCTL_SYSPLLCFG0_RDIVCLK2X_MASK) |
		((pllConfig.rDivClk1 << SYSCTL_SYSPLLCFG0_RDIVCLK1_OFS) & SYSCTL_SYSPLLCFG0_RDIVCLK1_MASK) |
		((pllConfig.rDivClk0 << SYSCTL_SYSPLLCFG0_RDIVCLK0_OFS) & SYSCTL_SYSPLLCFG0_RDIVCLK0_MASK) |
		pllConfig.enableCLK2x |
		pllConfig.enableCLK1 |
		pllConfig.enableCLK0 |
		(uint32_t)pllConfig.sysPLLMCLK),
		(SYSCTL_SYSPLLCFG0_RDIVCLK2X_MASK |
		SYSCTL_SYSPLLCFG0_RDIVCLK1_MASK |
		SYSCTL_SYSPLLCFG0_RDIVCLK0_MASK |
		SYSCTL_SYSPLLCFG0_ENABLECLK2X_MASK |
		SYSCTL_SYSPLLCFG0_ENABLECLK1_MASK |
		SYSCTL_SYSPLLCFG0_ENABLECLK0_MASK |
		SYSCTL_SYSPLLCFG0_MCLK2XVCO_MASK));

	DL_SYSCTL_enableSYSPLL();
	if (G3507_WaitClockStatus(SYSCTL_CLKSTATUS_SYSPLLGOOD_MASK,
		DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD,
		G3507_PLL_LOCK_TIMEOUT) == 0U)
	{
		return 0U;
	}

	return 1U;
}

static uint8_t G3507_SwitchMclkToSysPll80(void)
{
	uint32_t timeout;

	DL_SYSCTL_setHSCLKSource(DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
	if (G3507_WaitClockStatus(SYSCTL_CLKSTATUS_HSCLKGOOD_MASK,
		DL_SYSCTL_CLK_STATUS_HSCLK_GOOD,
		G3507_HSCLK_SWITCH_TIMEOUT) == 0U)
	{
		return 0U;
	}

	SYSCTL->SOCLOCK.MCLKCFG |= SYSCTL_MCLKCFG_USEHSCLK_ENABLE;

	timeout = G3507_HSCLK_SWITCH_TIMEOUT;
	while (timeout > 0UL)
	{
		if (DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_HSCLK)
		{
			return 1U;
		}
		timeout--;
	}

	return 0U;
}
#endif

/* EXTI 线到 NVIC 中断通道映射。 */
#define SYS_EXTI0_IRQn      (6U)
#define SYS_EXTI1_IRQn      (7U)
#define SYS_EXTI2_IRQn      (8U)
#define SYS_EXTI3_IRQn      (9U)
#define SYS_EXTI4_IRQn      (10U)
#define SYS_EXTI9_5_IRQn    (23U)
#define SYS_EXTI15_10_IRQn  (40U)

uint8_t SYS_EXTI_GetLineIndex(uint32_t pin)
{
	uint8_t index;

	for (index = 0U; index < 16U; ++index)
	{
		if (pin == (uint32_t)(1UL << index))
		{
			return index;
		}
	}

	return 0xFFU;
}

void SYS_Init(void)
{
	#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	static uint8_t s_clockInited = 0U;
	DL_SYSCTL_RESET_CAUSE resetCause;

	if (s_clockInited != 0U)
	{
		return;
	}

	resetCause = DL_SYSCTL_getResetCause();
	if ((resetCause == DL_SYSCTL_RESET_CAUSE_POR_HW_FAILURE) ||
		(resetCause == DL_SYSCTL_RESET_CAUSE_POR_EXTERNAL_NRST) ||
		(resetCause == DL_SYSCTL_RESET_CAUSE_POR_SW_TRIGGERED) ||
		(resetCause == DL_SYSCTL_RESET_CAUSE_BOR_SUPPLY_FAILURE) ||
		(resetCause == DL_SYSCTL_RESET_CAUSE_BOR_WAKE_FROM_SHUTDOWN))
	{
		/* 冷上电路径给模拟电源和内部基准更多稳定时间。 */
		G3507_BusyWaitCycles(G3507_STARTUP_SETTLE_CYCLES_COLD);
	}
	else
	{
		G3507_BusyWaitCycles(G3507_STARTUP_SETTLE_CYCLES_FAST);
	}

	G3507_WaitPowerGood();

	/*
	 * G3507 时钟策略：
	 * 1) 进入 RUN0，确保 MCLK 走高速域（非 LFCLK 低功耗模式）
	 * 2) 关闭 MCLK 分频并强制 SYSOSC=32MHz（BASE）
	 * 3) 默认启用 SYSPLL，将 MCLK 提升到 80MHz
	 */
	DL_SYSCTL_setPowerPolicyRUN0SLEEP0();
	DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);

	#if (G3507_ENABLE_PLL80 != 0U)
	{
		/* MCLK 使用 HSCLK/SYSPLL 时，需要手动设置 Flash wait state。 */
		DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

		if ((G3507_ConfigSysPll80WithTimeout() == 0U) ||
			(G3507_SwitchMclkToSysPll80() == 0U))
		{
			/* 强制策略：80MHz 初始化失败则立即系统复位重试，不回退 32MHz。 */
			DL_SYSCTL_resetDevice(DL_SYSCTL_RESET_SYSRST);
			while (1)
			{
			}
		}

		DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
	}
	#endif

	s_clockInited = 1U;
	#else
	/* 非 G3507 平台保持现状。 */
	#endif
}

uint32_t SYS_GetMclkHz(void)
{
	#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	uint32_t sourceHz;
	uint32_t divider;

	if (DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_LFCLK)
	{
		return 32768UL;
	}

	if (DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_HSCLK)
	{
		if (DL_SYSCTL_getHSCLKSource() == DL_SYSCTL_HSCLK_SOURCE_SYSPLL)
		{
			sourceHz = 80000000UL;
		}
		else
		{
			sourceHz = 32000000UL;
		}
	}
	else
	{
		if (DL_SYSCTL_getCurrentSYSOSCFreq() == DL_SYSCTL_SYSOSC_FREQ_4M)
		{
			sourceHz = 4000000UL;
		}
		else
		{
			sourceHz = 32000000UL;
		}
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
	#else
	return 0UL;
	#endif
}

uint32_t SYS_GetBusClkHz(void)
{
	#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	uint32_t mclkHz;
	DL_SYSCTL_ULPCLK_DIV ulpDiv;

	mclkHz = SYS_GetMclkHz();
	ulpDiv = DL_SYSCTL_getULPCLKDivider();

	if (ulpDiv == DL_SYSCTL_ULPCLK_DIV_2)
	{
		return (mclkHz / 2UL);
	}
	if (ulpDiv == DL_SYSCTL_ULPCLK_DIV_3)
	{
		return (mclkHz / 3UL);
	}
	return mclkHz;
	#else
	return 0UL;
	#endif
}

uint32_t SYS_GetResetCause(void)
{
	#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	return (uint32_t)DL_SYSCTL_getResetCause();
	#else
	return 0UL;
	#endif
}

uint32_t SYS_EXTI_GetIrqn(void *port, uint32_t pin)
{
	uint8_t lineIndex;

	if ((port == 0) || (pin == 0U))
	{
		return SYS_EXTI_INVALID_IRQN;
	}

	lineIndex = SYS_EXTI_GetLineIndex(pin);
	if (lineIndex > 15U)
	{
		return SYS_EXTI_INVALID_IRQN;
	}

	#if (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
	if (port == GPIOA)
	{
		return (uint32_t)GPIOA_INT_IRQn;
	}
	if (port == GPIOB)
	{
		return (uint32_t)GPIOB_INT_IRQn;
	}
	return SYS_EXTI_INVALID_IRQN;
	#else
	if (lineIndex == 0U)
	{
		return (uint32_t)SYS_EXTI0_IRQn;
	}
	if (lineIndex == 1U)
	{
		return (uint32_t)SYS_EXTI1_IRQn;
	}
	if (lineIndex == 2U)
	{
		return (uint32_t)SYS_EXTI2_IRQn;
	}
	if (lineIndex == 3U)
	{
		return (uint32_t)SYS_EXTI3_IRQn;
	}
	if (lineIndex == 4U)
	{
		return (uint32_t)SYS_EXTI4_IRQn;
	}
	if ((lineIndex >= 5U) && (lineIndex <= 9U))
	{
		return (uint32_t)SYS_EXTI9_5_IRQn;
	}
	if ((lineIndex >= 10U) && (lineIndex <= 15U))
	{
		return (uint32_t)SYS_EXTI15_10_IRQn;
	}
	return SYS_EXTI_INVALID_IRQN;
	#endif
}

uint8_t SYS_EXTI_LineInGroup(uint32_t pin, uint8_t startLine, uint8_t endLine)
{
	uint8_t lineIndex;

	lineIndex = SYS_EXTI_GetLineIndex(pin);
	if ((lineIndex > 15U) || (lineIndex < startLine) || (lineIndex > endLine))
	{
		return 0U;
	}

	return 1U;
}

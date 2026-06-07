#include "G3507_sys.h"

#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "ti/driverlib/dl_common.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti/driverlib/m0p/dl_sysctl.h"

/* G3507 时钟策略：默认启用 SYSPLL 提升到 80MHz。 */
#ifndef G3507_ENABLE_PLL80
#define G3507_ENABLE_PLL80 (1U)
#endif

#define G3507_STARTUP_SETTLE_CYCLES_FAST   (80000UL)
#define G3507_STARTUP_SETTLE_CYCLES_COLD   (800000UL)   /* 冷启动延长到约 25ms@32MHz，确保模拟电路充分稳定 */
#define G3507_POWER_GOOD_TIMEOUT           (800000UL)
#define G3507_PLL_LOCK_TIMEOUT             (1600000UL)  /* 冷启动 PLL 锁定给更多时间 */
#define G3507_HSCLK_SWITCH_TIMEOUT         (600000UL)
#define G3507_SYSOSC_SETTLE_CYCLES         (200000UL)   /* SYSOSC 频率切换后稳定延时 */

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

/* 初始化系统时钟 - 设置 G3507 主频为 80MHz 且保证断电再上电能自动启动 */
void G3507_SYS_Init(void)
{
	static uint8_t s_clockInited = 0U;
	DL_SYSCTL_RESET_CAUSE resetCause;
	uint8_t pllOk;
	uint8_t retry;

	if (s_clockInited != 0U)
	{
		return;
	}

	/* 读取复位原因，用于决定冷/热启动等待时长。 */
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
	 * 1) setPowerPolicyRUN0SLEEP0：MCLK→LFCLK(32kHz)，安全状态下改时钟配置
	 * 2) 设 SYSOSC=32MHz，立即切回 MCLK=SYSOSC（否则后续 BusyWait 全在 32kHz）
	 * 3) 等 SYSOSC 稳定 + 等电源就绪
	 * 4) VBOOST 常开，保证 80MHz 下核心电压不跌落
	 * 5) 配 PLL 80MHz → 切 HSCLK
	 * 6) 冷启动 PLL 失败 → 做一次系统复位 → 仍失败降级 32MHz
	 */
	DL_SYSCTL_setPowerPolicyRUN0SLEEP0();   /* MCLK→LFCLK 32kHz，安全配置态 */
	DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);

	/*
	 * 切回 MCLK=SYSOSC(32MHz)。此操作必须在所有 BusyWait 之前，
	 * 否则延时循环在 32kHz LFCLK 下耗时是预期的 1000 倍。
	 */
	SYSCTL->SOCLOCK.MCLKCFG &= ~SYSCTL_MCLKCFG_USELFCLK_ENABLE;

	/* SYSOSC 切到 32MHz 后等振荡器稳定，约 6ms@32MHz。 */
	G3507_BusyWaitCycles(G3507_SYSOSC_SETTLE_CYCLES);

	/*
	 * VBOOST（模拟电荷泵）常开，确保 80MHz 运行时代核电压足够。
	 * 冷启动时如果 VBOOST 未开，PLL 锁定时核电压可能跌落导致崩溃。
	 */
	DL_SYSCTL_setVBOOSTConfig(DL_SYSCTL_VBOOST_ONALWAYS);

	#if (G3507_ENABLE_PLL80 != 0U)
	{
		/*
		 * 强制 80MHz 策略：PLL 失败就系统复位重试。
		 * 不降级——32MHz 会导致串口乱码、I2C 时序错乱。
		 *
		 * 冷启动时 PMU 未充分暖机，第一次可能失败但复位后通常成功。
		 * 现在 MCLK 已切回 32MHz + VBOOST 已开，成功率应大幅提升。
		 */
		pllOk = 0U;

		/* MCLK 使用 HSCLK/SYSPLL 时，需要手动设置 Flash wait state。 */
		DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

		for (retry = 0U; retry < 3U; ++retry)
		{
			if (G3507_ConfigSysPll80WithTimeout() != 0U)
			{
				if (G3507_SwitchMclkToSysPll80() != 0U)
				{
					pllOk = 1U;
					break;
				}
			}
			DL_SYSCTL_disableSYSPLL();
			G3507_BusyWaitCycles(50000UL);
		}

		if (pllOk != 0U)
		{
			DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
		}
		else
		{
			/* 80MHz 失败 → 复位重试，不复位永远到不了 80MHz */
			DL_SYSCTL_disableSYSPLL();
			DL_SYSCTL_resetDevice(DL_SYSCTL_RESET_SYSRST);
			while (1) { /* 等复位生效 */ }
		}
	}
	#endif

	s_clockInited = 1U;
}

uint32_t G3507_SYS_GetMclkHz(void)
{
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
}

uint32_t G3507_SYS_GetBusClkHz(void)
{
	uint32_t mclkHz;
	DL_SYSCTL_ULPCLK_DIV ulpDiv;

	mclkHz = G3507_SYS_GetMclkHz();
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
}

uint32_t G3507_SYS_GetResetCause(void)
{
	return (uint32_t)DL_SYSCTL_getResetCause();
}

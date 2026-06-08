#include "G3507_Encoder.h"

#include "G3507_gpio.h"
#include "G3507_exti.h"
#include "gpio.h"
#include "IrqPriority.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"

/*
 * G3507 编码器底层实现：
 * - 使用 GPIO 外部中断（上升沿触发）模拟正交编码器；
 * - 两路信号分别配置 EXTI；
 * - 中断中读取另一相电平判断转动方向，累加到内部计数器；
 * - GetCount 返回累加值并清零，供上层在周期任务中读取。
 */

#define G3507_ENCODER_MAX  (2U)

/* 单路编码器的硬件上下文 */
typedef struct
{
	uint8_t  active;       /* 是否已初始化 */
	void    *portA;
	uint32_t pinA;
	void    *portB;
	uint32_t pinB;
} G3507_Encoder_Ctx_t;

static G3507_Encoder_Ctx_t s_ctx[G3507_ENCODER_MAX];

/* 每个编码器的累加计数值 */
static volatile int32_t s_encoderCount[G3507_ENCODER_MAX];

void G3507_Encoder_Init(uint8_t coreId)
{
	G3507_Encoder_Ctx_t *ctx;

	if (coreId >= G3507_ENCODER_MAX)
	{
		return;
	}

	ctx = &s_ctx[coreId];
	if (ctx->active != 0U)
	{
		return;
	}

	/* 1) 配置两路 GPIO 为上拉输入（由 G3507_EXTI_Init 内部完成）*/

	/* 2) 配置 EXTI：根据端口选择正确的 IRQn */
	{
		IRQn_Type irqA = ((ctx->portA == GPIOA) ? GPIOA_INT_IRQn : GPIOB_INT_IRQn);
		IRQn_Type irqB = ((ctx->portB == GPIOB) ? GPIOB_INT_IRQn : GPIOA_INT_IRQn);

		G3507_EXTI_Init(ctx->portA, ctx->pinA, 0x01U, /* rising */
		                (uint32_t)irqA, IRQ_PRIO_ENCODER, IRQ_SUB_PRIO_ENCODER);
		G3507_EXTI_Init(ctx->portB, ctx->pinB, 0x01U, /* rising */
		                (uint32_t)irqB, IRQ_PRIO_ENCODER, IRQ_SUB_PRIO_ENCODER);
	}

	/* 3) 开启施密特迟滞：滤除同端口软件 I2C 翻转的高频噪声 */
	{
		uint32_t iomuxA = G3507_GetIomux((GPIO_Regs *)ctx->portA, ctx->pinA);
		uint32_t iomuxB = G3507_GetIomux((GPIO_Regs *)ctx->portB, ctx->pinB);

		if (iomuxA != 0xFFFFFFFFUL) {
			IOMUX->SECCFG.PINCM[iomuxA] &= ~IOMUX_PINCM_HYSTEN_MASK;
			IOMUX->SECCFG.PINCM[iomuxA] |= IOMUX_PINCM_HYSTEN_ENABLE;
		}
		if (iomuxB != 0xFFFFFFFFUL) {
			IOMUX->SECCFG.PINCM[iomuxB] &= ~IOMUX_PINCM_HYSTEN_MASK;
			IOMUX->SECCFG.PINCM[iomuxB] |= IOMUX_PINCM_HYSTEN_ENABLE;
		}
	}

	/* 清中断挂起位 */
	s_encoderCount[coreId] = 0;
	ctx->active = 1U;
}

/*
 * 为编码器设置端口和引脚信息。
 * 由 API 层在 Init 之前调用，填充上下文。
 */
void G3507_Encoder_SetPins(uint8_t coreId,
                           void *portA, uint32_t pinA,
                           void *portB, uint32_t pinB)
{
	if (coreId >= G3507_ENCODER_MAX)
	{
		return;
	}

	s_ctx[coreId].portA  = portA;
	s_ctx[coreId].pinA   = pinA;
	s_ctx[coreId].portB  = portB;
	s_ctx[coreId].pinB   = pinB;
}

/*
 * 在 GROUP1_IRQHandler 中调用，处理 GPIOA/GPIOB 上的编码器中断。
 * 逻辑：
 * - 扫描所有已注册编码器的两个相位引脚；
 * - 如果某引脚有 pending 中断 → 读取另一相电平 → 更新方向计数；
 * - 清除该引脚的中断标志。
 */
void G3507_Encoder_ProcessPortIrq(void *port)
{
	uint8_t i;

	if (port == 0)
	{
		return;
	}

	for (i = 0U; i < G3507_ENCODER_MAX; ++i)
	{
		G3507_Encoder_Ctx_t *ctx = &s_ctx[i];

		if (ctx->active == 0U)
		{
			continue;
		}

		/* 处理 A 相中断 */
		if (ctx->portA == port)
		{
			if (DL_GPIO_getEnabledInterruptStatus((GPIO_Regs *)ctx->portA, ctx->pinA) != 0U)
			{
				/* A 相触发：读 B 相电平 */
				if (API_GPIO_Read(ctx->portB, ctx->pinB) == 0U)
				{
					s_encoderCount[i]--;
				}
				else
				{
					s_encoderCount[i]++;
				}

				DL_GPIO_clearInterruptStatus((GPIO_Regs *)ctx->portA, ctx->pinA);
			}
		}

		/* 处理 B 相中断 */
		if (ctx->portB == port)
		{
			if (DL_GPIO_getEnabledInterruptStatus((GPIO_Regs *)ctx->portB, ctx->pinB) != 0U)
			{
				/* B 相触发：读 A 相电平 */
				if (API_GPIO_Read(ctx->portA, ctx->pinA) == 0U)
				{
					s_encoderCount[i]++;
				}
				else
				{
					s_encoderCount[i]--;
				}

				DL_GPIO_clearInterruptStatus((GPIO_Regs *)ctx->portB, ctx->pinB);
			}
		}
	}
}

int16_t G3507_Encoder_GetCount(uint8_t coreId)
{
	int32_t count;
	int16_t result;

	if (coreId >= G3507_ENCODER_MAX)
	{
		return 0;
	}

	/* 原子读-清零：用临界区保护（M0+ 无 ldrex/strex，关全局中断最简单） */
	__disable_irq();
	count  = s_encoderCount[coreId];
	s_encoderCount[coreId] = 0;
	__enable_irq();

	/* 截断到 int16_t */
	if (count > 32767L)
	{
		result = 32767;
	}
	else if (count < -32768L)
	{
		result = -32768;
	}
	else
	{
		result = (int16_t)count;
	}

	return result;
}

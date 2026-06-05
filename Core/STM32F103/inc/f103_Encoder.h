#ifndef __F103_ENCODER_H
#define __F103_ENCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * F103 编码器底层驱动：
 * - 使用定时器编码器模式（TIM2 / TIM4）；
 * - 两路正交信号经 TIM_CH1 + TIM_CH2 输入；
 * - 计数器自动累加方向计数，无需中断。
 */

/* coreId 即 TIM 编号：TIM2=1, TIM4=3 */
void    F103_Encoder_Init(uint8_t coreId);
int16_t F103_Encoder_GetCount(uint8_t coreId);

#ifdef __cplusplus
}
#endif

#endif /* __F103_ENCODER_H */

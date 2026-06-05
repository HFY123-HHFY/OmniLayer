#ifndef __F103_ENCODER_H
#define __F103_ENCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * F103 编码器底层驱动：
 * - 接收 portA/pinA、portB/pinB 参数，不硬编码引脚；
 * - 自动根据端口地址使能 GPIO 时钟，根据 coreId 使能 TIM 时钟；
 * - 配置 GPIO 为上拉输入，TIM 编码器模式 3。
 */

/*
 * 初始化编码器：
 * coreId: TIM 编号（1=TIM2, 3=TIM4）
 * portA/pinA: A 相 GPIO（对应 TIM_CH1）
 * portB/pinB: B 相 GPIO（对应 TIM_CH2）
 */
void    F103_Encoder_Init(uint8_t coreId,
                          void *portA, uint32_t pinA,
                          void *portB, uint32_t pinB);

int16_t F103_Encoder_GetCount(uint8_t coreId);

#ifdef __cplusplus
}
#endif

#endif /* __F103_ENCODER_H */

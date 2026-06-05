#ifndef __G3507_ENCODER_H
#define __G3507_ENCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * G3507 编码器底层驱动：
 * - 使用 GPIO 外部中断模拟正交编码器；
 * - 两路信号分别配置为上升沿触发；
 * - 在中断中读取另一相电平判断方向，累加到内部计数器；
 * - GetCount 返回累加值并清零，供上层周期读取。
 */

/* coreId: 0 = Encoder 1, 1 = Encoder 2 */

/*
 * 设置编码器两相引脚信息（必须在 Init 之前调用）。
 */
void G3507_Encoder_SetPins(uint8_t coreId,
                           void *portA, uint32_t pinA,
                           void *portB, uint32_t pinB);

void    G3507_Encoder_Init(uint8_t coreId);
int16_t G3507_Encoder_GetCount(uint8_t coreId);

/*
 * 在 GROUP1_IRQHandler 中调用，处理 GPIOA/GPIOB 上的编码器中断。
 * port 为 GPIOA 或 GPIOB。
 */
void G3507_Encoder_ProcessPortIrq(void *port);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_ENCODER_H */

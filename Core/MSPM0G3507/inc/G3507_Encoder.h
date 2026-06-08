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
 * - 在中断中读取另一相电平判断方向，累加到 raw 计数器；
 * - SnapshotAll 原子快照 raw→stable 并清零 raw（在定时器 ISR 调用）；
 * - GetStable 返回 stable 值（在 main loop 调用）。
 *
 * 数据流（仿 Keil 双缓冲）：
 *   EXTI ISR → s_encoderRaw[±1]
 *   TIM2 ISR → SnapshotAll: raw→stable, raw=0  (固定 20ms 窗口)
 *   main    → GetStable → 速度计算
 */

/* coreId: 0 = Encoder 1, 1 = Encoder 2 */

/*
 * 设置编码器两相引脚信息（必须在 Init 之前调用）。
 */
void G3507_Encoder_SetPins(uint8_t coreId,
                           void *portA, uint32_t pinA,
                           void *portB, uint32_t pinB);

void    G3507_Encoder_Init(uint8_t coreId);
int16_t G3507_Encoder_GetCount(uint8_t coreId);    /* 返回 stable 快照值（不再清零） */
int16_t G3507_Encoder_GetStable(uint8_t coreId);   /* 同 GetCount，语义更明确 */

/*
 * 原子快照所有编码器：raw→stable，清零 raw。
 * 应在固定周期的定时器 ISR 中调用，确保速度采样的时间窗口恒定。
 */
void G3507_Encoder_SnapshotAll(void);

/*
 * 在 GROUP1_IRQHandler 中调用，处理 GPIOA/GPIOB 上的编码器中断。
 * port 为 GPIOA 或 GPIOB。
 */
void G3507_Encoder_ProcessPortIrq(void *port);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_ENCODER_H */

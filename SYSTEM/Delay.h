#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 统一延时接口（对上层模块统一暴露）：
 * 1) Delay_us: 微秒级阻塞延时
 * 2) Delay_ms: 毫秒级阻塞延时
 * 3) Delay_s : 秒级阻塞延时
 *
 * 说明：
 * - 上层只需要 include 本头文件，不需要关心具体 MCU 型号。
 * - 实际实现由 Core/STM32F103/f103_delay.c 或
 *   Core/STM32F407/f407_delay.c 提供。
 */
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */

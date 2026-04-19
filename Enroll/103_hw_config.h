#ifndef __103_HW_CONFIG_H
#define __103_HW_CONFIG_H

#include "LED.h"

/*
 * 103_hw_config.h 板级硬件映射宏
 */

/*
 * HW_LED_MAP(X) 的用途：
 * 1) X 是一个宏函数，用于把每个 LED 映射项展开成结构体初始化代码。
 * 2) 这样 Enroll.c 里只需要写一次模板，不用重复手写每个 LED 项。
 */
#define HW_LED_MAP(X) \
	X(LED1, GPIOC, GPIO_Pin_13)

/* 当前板子上注册了 1 个 LED。 */
#define HW_LED_COUNT  1U

#endif /* __103_HW_CONFIG_H */

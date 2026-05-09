#ifndef __G3507_HW_CONFIG_H
#define __G3507_HW_CONFIG_H

#include "LED.h"
#include "ti/driverlib/dl_gpio.h"

/*
 * G3507_hw_config.h
 * 只维护实际用到的端口/引脚/IOMUX，注册和查找都在这里完成。
 */

/* LED 板级映射*/
#define HW_LED_MAP(X) \
    X(LED1, GPIOA, DL_GPIO_PIN_14)
#define G3507_LED1_IOMUX  IOMUX_PINCM36 // GPIOX 对应的 IOMUX

/* 当前板子上注册了 1 个 LED */
#define HW_LED_COUNT  1U

#endif /* __G3507_HW_CONFIG_H */

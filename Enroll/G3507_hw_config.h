#ifndef __G3507_HW_CONFIG_H
#define __G3507_HW_CONFIG_H

#include "LED.h"
#include "usart.h"
#include "tim.h"
#include "ti/driverlib/dl_gpio.h"

/*
 * G3507_hw_config.h
 * 只维护实际用到的端口/引脚/IOMUX，注册和查找都在这里完成。
 */

/* LED 板级映射 */
#define HW_LED_MAP(X) \
    X(LED1, GPIOB, DL_GPIO_PIN_22)
#define G3507_LED1_IOMUX  IOMUX_PINCM50

/* USART0 板级映射（TX=PA10, RX=PA11） */
#define HW_USART0_TX_PORT GPIOA
#define HW_USART0_TX_PIN  DL_GPIO_PIN_10
#define HW_USART0_RX_PORT GPIOA
#define HW_USART0_RX_PIN  DL_GPIO_PIN_11

#define G3507_USART0_TX_IOMUX IOMUX_PINCM21
#define G3507_USART0_TX_FUNC  IOMUX_PINCM21_PF_UART0_TX
#define G3507_USART0_RX_IOMUX IOMUX_PINCM22
#define G3507_USART0_RX_FUNC  IOMUX_PINCM22_PF_UART0_RX

#define HW_USART_MAP(X) \
    X(API_USART1, API_USART_CORE_UART0, HW_USART0_TX_PORT, HW_USART0_TX_PIN, HW_USART0_RX_PORT, HW_USART0_RX_PIN)

/* TIM 板级映射：逻辑 API_TIM1 绑定到硬件 TIMG0。 */
#define HW_TIM_MAP(X) \
    X(API_TIM1, API_TIM_CORE_TIMG0)

/* 当前板子上注册了 1 个 LED */
#define HW_LED_COUNT    1U
/* 当前板子上注册了 1 路 USART */
#define HW_USART_COUNT  1U
/* 当前板子上注册了 1 路 TIM */
#define HW_TIM_COUNT    1U

#endif /* __G3507_HW_CONFIG_H */

#ifndef __F103_GPIO_H
#define __F103_GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	/* 配置寄存器低 8 位引脚（Pin0~Pin7）。 */
	volatile uint32_t CRL;
	/* 配置寄存器高 8 位引脚（Pin8~Pin15）。 */
	volatile uint32_t CRH;
	/* 输入数据寄存器。 */
	volatile uint32_t IDR;
	/* 输出数据寄存器。 */
	volatile uint32_t ODR;
	/* 置位寄存器：写 1 置位对应引脚。 */
	volatile uint32_t BSRR;
	/* 复位寄存器：写 1 复位对应引脚。 */
	volatile uint32_t BRR;
	/* 配置锁定寄存器。 */
	volatile uint32_t LCKR;
} F103_GPIO_Regs_t;

typedef struct
{
	/* RCC 控制寄存器。 */
	volatile uint32_t CR;
	/* 时钟配置寄存器。 */
	volatile uint32_t CFGR;
	/* 时钟中断寄存器。 */
	volatile uint32_t CIR;
	/* APB2 外设复位寄存器。 */
	volatile uint32_t APB2RSTR;
	/* APB1 外设复位寄存器。 */
	volatile uint32_t APB1RSTR;
	/* AHB 外设使能寄存器。 */
	volatile uint32_t AHBENR;
	/* APB2 外设使能寄存器。 */
	volatile uint32_t APB2ENR;
	/* APB1 外设使能寄存器。 */
	volatile uint32_t APB1ENR;
	/* 备份域控制寄存器。 */
	volatile uint32_t BDCR;
	/* 控制/状态寄存器。 */
	volatile uint32_t CSR;
} F103_RCC_Regs_t;

#define F103_RCC_BASE   (0x40021000UL)
#define F103_GPIOA_BASE (0x40010800UL)
#define F103_GPIOB_BASE (0x40010C00UL)
#define F103_GPIOC_BASE (0x40011000UL)
#define F103_GPIOD_BASE (0x40011400UL)
#define F103_GPIOE_BASE (0x40011800UL)

#define F103_RCC   ((F103_RCC_Regs_t *)F103_RCC_BASE)
#define GPIOA      ((F103_GPIO_Regs_t *)F103_GPIOA_BASE)
#define GPIOB      ((F103_GPIO_Regs_t *)F103_GPIOB_BASE)
#define GPIOC      ((F103_GPIO_Regs_t *)F103_GPIOC_BASE)
#define GPIOD      ((F103_GPIO_Regs_t *)F103_GPIOD_BASE)
#define GPIOE      ((F103_GPIO_Regs_t *)F103_GPIOE_BASE)

#define GPIO_Pin_0   ((uint16_t)0x0001U)
#define GPIO_Pin_1   ((uint16_t)0x0002U)
#define GPIO_Pin_2   ((uint16_t)0x0004U)
#define GPIO_Pin_3   ((uint16_t)0x0008U)
#define GPIO_Pin_4   ((uint16_t)0x0010U)
#define GPIO_Pin_5   ((uint16_t)0x0020U)
#define GPIO_Pin_6   ((uint16_t)0x0040U)
#define GPIO_Pin_7   ((uint16_t)0x0080U)
#define GPIO_Pin_8   ((uint16_t)0x0100U)
#define GPIO_Pin_9   ((uint16_t)0x0200U)
#define GPIO_Pin_10  ((uint16_t)0x0400U)
#define GPIO_Pin_11  ((uint16_t)0x0800U)
#define GPIO_Pin_12  ((uint16_t)0x1000U)
#define GPIO_Pin_13  ((uint16_t)0x2000U)
#define GPIO_Pin_14  ((uint16_t)0x4000U)
#define GPIO_Pin_15  ((uint16_t)0x8000U)

/* 根据端口地址打开对应 APB2 GPIO 时钟。 */
void F103_GPIO_EnablePortClock(void *port);
/* 把单 bit 引脚掩码转换为引脚编号（0~15）。 */
uint32_t F103_GPIO_PinIndex(uint16_t pin);

/* GPIO 输出初始化：当前实现为推挽输出 2MHz。 */
void F103_GPIO_InitOutput(void *port, uint16_t pin);
/* GPIO 输入初始化：当前实现为浮空输入。 */
void F103_GPIO_InitInput(void *port, uint16_t pin);
/* GPIO 上拉输入初始化：用于按键等低电平触发输入。 */
void F103_GPIO_InitInputPullUp(void *port, uint16_t pin);
/* GPIO 写电平：level 非 0 写高，0 写低。 */
void F103_GPIO_Write(void *port, uint16_t pin, uint8_t level);
/* GPIO 读输入电平：返回 1 表示高电平，0 表示低电平。 */
uint8_t F103_GPIO_Read(void *port, uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __F103_GPIO_H */

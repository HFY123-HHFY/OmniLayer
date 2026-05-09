#ifndef __F407_GPIO_H
#define __F407_GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	/* 模式寄存器：每个引脚 2bit。 */
	volatile uint32_t MODER;
	/* 输出类型寄存器：每个引脚 1bit。 */
	volatile uint32_t OTYPER;
	/* 输出速度寄存器：每个引脚 2bit。 */
	volatile uint32_t OSPEEDR;
	/* 上拉下拉寄存器：每个引脚 2bit。 */
	volatile uint32_t PUPDR;
	/* 输入数据寄存器。 */
	volatile uint32_t IDR;
	/* 输出数据寄存器。 */
	volatile uint32_t ODR;
	/* 置位/复位寄存器。 */
	volatile uint32_t BSRR;
	/* 配置锁定寄存器。 */
	volatile uint32_t LCKR;
	/* 复用功能低寄存器（Pin0~Pin7）。 */
	volatile uint32_t AFRL;
	/* 复用功能高寄存器（Pin8~Pin15）。 */
	volatile uint32_t AFRH;
} F407_GPIO_Regs_t;

typedef struct
{
	/* RCC 控制寄存器。 */
	volatile uint32_t CR;
	/* PLL 配置寄存器。 */
	volatile uint32_t PLLCFGR;
	/* 时钟配置寄存器。 */
	volatile uint32_t CFGR;
	/* 时钟中断寄存器。 */
	volatile uint32_t CIR;
	/* AHB1 外设复位寄存器。 */
	volatile uint32_t AHB1RSTR;
	/* AHB2 外设复位寄存器。 */
	volatile uint32_t AHB2RSTR;
	/* AHB3 外设复位寄存器。 */
	volatile uint32_t AHB3RSTR;
	/* 保留。 */
	volatile uint32_t RESERVED0;
	/* APB1 外设复位寄存器。 */
	volatile uint32_t APB1RSTR;
	/* APB2 外设复位寄存器。 */
	volatile uint32_t APB2RSTR;
	/* 保留。 */
	volatile uint32_t RESERVED1[2];
	/* AHB1 外设时钟使能寄存器。 */
	volatile uint32_t AHB1ENR;
	/* AHB2 外设时钟使能寄存器。 */
	volatile uint32_t AHB2ENR;
	/* AHB3 外设时钟使能寄存器。 */
	volatile uint32_t AHB3ENR;
	/* 保留。 */
	volatile uint32_t RESERVED2;
	/* APB1 外设时钟使能寄存器。 */
	volatile uint32_t APB1ENR;
	/* APB2 外设时钟使能寄存器。 */
	volatile uint32_t APB2ENR;
} F407_RCC_Regs_t;

#define F407_RCC_BASE   (0x40023800UL)
#define F407_GPIOA_BASE (0x40020000UL)
#define F407_GPIOB_BASE (0x40020400UL)
#define F407_GPIOC_BASE (0x40020800UL)
#define F407_GPIOD_BASE (0x40020C00UL)
#define F407_GPIOE_BASE (0x40021000UL)
#define F407_GPIOF_BASE (0x40021400UL)
#define F407_GPIOG_BASE (0x40021800UL)
#define F407_GPIOH_BASE (0x40021C00UL)
#define F407_GPIOI_BASE (0x40022000UL)

#define F407_RCC   ((F407_RCC_Regs_t *)F407_RCC_BASE)
#define GPIOA      ((F407_GPIO_Regs_t *)F407_GPIOA_BASE)
#define GPIOB      ((F407_GPIO_Regs_t *)F407_GPIOB_BASE)
#define GPIOC      ((F407_GPIO_Regs_t *)F407_GPIOC_BASE)
#define GPIOD      ((F407_GPIO_Regs_t *)F407_GPIOD_BASE)
#define GPIOE      ((F407_GPIO_Regs_t *)F407_GPIOE_BASE)
#define GPIOF      ((F407_GPIO_Regs_t *)F407_GPIOF_BASE)
#define GPIOG      ((F407_GPIO_Regs_t *)F407_GPIOG_BASE)
#define GPIOH      ((F407_GPIO_Regs_t *)F407_GPIOH_BASE)
#define GPIOI      ((F407_GPIO_Regs_t *)F407_GPIOI_BASE)

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

/* 根据端口地址打开对应 AHB1 GPIO 时钟。 */
void F407_GPIO_EnablePortClock(void *port);
/* 把单 bit 引脚掩码转换为引脚编号（0~15）。 */
uint32_t F407_GPIO_PinIndex(uint32_t pin);

/* GPIO 输出初始化：当前实现为推挽输出低速模式。 */
void F407_GPIO_InitOutput(void *port, uint32_t pin);
/* GPIO 输入初始化：当前实现为无上下拉输入。 */
void F407_GPIO_InitInput(void *port, uint32_t pin);
/* GPIO 上拉输入初始化：用于按键等低电平触发输入。 */
void F407_GPIO_InitInputPullUp(void *port, uint32_t pin);
/* GPIO 写电平：level 非 0 写高，0 写低。 */
void F407_GPIO_Write(void *port, uint32_t pin, uint8_t level);
/* GPIO 读输入电平：返回 1 表示高电平，0 表示低电平。 */
uint8_t F407_GPIO_Read(void *port, uint32_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __F407_GPIO_H */

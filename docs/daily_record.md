## 2026-05-09

### 1. G3507 的 IOMUX 该怎么理解

TI 的 MSPM0 不像 STM32 只改 GPIO 端口和引脚编号就够了。G3507 还要同时配置 PINCM/IOMUX，因为“这个引脚要不要输出、输出什么功能、对应哪一个封装脚”都要一起绑定。

当前工程里已经把这件事收敛到板级文件 [Enroll/G3507_hw_config.h](Enroll/G3507_hw_config.h) 了：

- `HW_LED_MAP(X)` 只负责给 BSP 的 LED 层用，保持和 STM32 风格一致。
- `HW_GPIO_IOMUX_MAP(X)` 专门给 G3507 Core GPIO 层用，保存 `port / pin / iomux` 三元组。
- Core 的 [Core/MSPM0G3507/src/G3507_gpio.c](Core/MSPM0G3507/src/G3507_gpio.c) 只读这份映射表，不再自己硬编码 `GPIOB + DL_GPIO_PIN_22 + IOMUX_PINCM50`。

换句话说，改 G3507 的 IO 时现在只要改板级配置文件里的那一行映射；IOMUX 仍然需要知道，但已经集中在一处，不再散落在 Core 里。

如果后面板子引脚变多，建议继续保持这种“三元组板级表”的方式，而不要把 IOMUX 再拆散到不同模块里。

### 2. API EXTI、Core EXTI、SYS 分别负责什么

现在这三层的职责是清晰分开的：

- API 层：统一外部中断接口。当前入口是 [API/inc/exti.h](API/inc/exti.h) 和 [API/src/exti.c](API/src/exti.c)。它只负责把“端口 + 引脚 + 触发沿 + 优先级”转发到当前 MCU 的 Core 实现。
- Core 层：每个 MCU 自己的 EXTI 底层实现。
	- F103 在 [Core/STM32F103/f103_sys.c](Core/STM32F103/f103_sys.c)
	- F407 在 [Core/STM32F407/f407_sys.c](Core/STM32F407/f407_sys.c)
	- 这里负责真正写 AFIO / SYSCFG / EXTI / NVIC 寄存器
- SYS 层：系统注册和转发层。当前入口是 [SYSTEM/sys.h](SYSTEM/sys.h) 和 [SYSTEM/sys.c](SYSTEM/sys.c)。
	- 它把“板级的某个中断脚”转换成“EXTI 线号”
	- 然后调用 API 层 EXTI 接口
	- 另外还提供 `SYS_EXTI_IRQHandlerGroup()` 方便中断服务函数按线组清 pending

### 3. 这些优先级到底去哪里改

你后面如果要改优先级，文件位置是这样的：

- 外部中断优先级：改 Core 层对应 MCU 的 EXTI 初始化文件
	- F103： [Core/STM32F103/f103_sys.c](Core/STM32F103/f103_sys.c)
	- F407： [Core/STM32F407/f407_sys.c](Core/STM32F407/f407_sys.c)
	- 目前优先级是从 `SYS_EXTI_Register(..., preemptPriority, subPriority)` 传进去，再写到 NVIC 寄存器
- 定时器中断优先级：改 Core 层对应 MCU 的 TIM 初始化文件
	- F103： [Core/STM32F103/src/f103_tim.c](Core/STM32F103/src/f103_tim.c)
	- F407： [Core/STM32F407/src/f407_tim.c](Core/STM32F407/src/f407_tim.c)
	- 当前 TIM 初始化只负责开 NVIC，中断优先级如果要加，应该在这两份文件里统一补上
- 串口中断优先级：改 Core 层对应 MCU 的 USART 初始化文件
	- F103： [Core/STM32F103/src/f103_usart.c](Core/STM32F103/src/f103_usart.c)
	- F407： [Core/STM32F407/src/f407_usart.c](Core/STM32F407/src/f407_usart.c)

### 4. 目前工程里的一个实际状态

- 你现在的 MPU6050 外部中断是通过 [BSP/MPU6050/MPU6050_Int.c](BSP/MPU6050/MPU6050_Int.c) 注册进 SYS 的。
- 业务层只需要关心“这个模块的 EXTI 线和触发方式”，不用直接碰 AFIO/SYSCFG/NVIC。
- G3507 的 EXTI 目前还没有完全接进来，所以这套收敛目前主要服务 F103/F407；G3507 后面可以沿同样模式补。

### 5. 这次结论

- STM32 侧的 IOMUX 问题不存在，因为它没有这个概念。
- G3507 的 IOMUX 不能省，但可以集中在板级映射表里，避免 Core 层和 BSP 层重复维护。
- EXTI / TIM / USART 的优先级都应该在各自 MCU 的 Core 初始化文件里统一改，SYS 层只负责“谁对应哪条线/哪个 IRQ”。

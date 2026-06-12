# OmniLayer 工程架构深度解析

> 本文档旨在帮助在多个 Claude Code 对话中快速恢复对工程架构的完整认知。
> 每次重新开启对话后，Claude Code 只需阅读本文档即可快速理解项目设计原则与编码约定。

---

## 1. 项目元信息

| 项目 | 详情 |
|------|------|
| **名称** | OmniLayer |
| **定位** | 多 MCU 嵌入式工程分层开发架构框架 |
| **构建工具** | CMake + GCC ARM Embedded + OpenOCD |
| **IDE 兼容** | VS Code (主) + Keil MDK (保留兼容) |
| **分支策略** | `main` (裸机主线) / `FreeRTOS` (RTOS 方向) |
| **作者** | Hu Fangyuan |
| **联系方式** | 634591772@qq.com |
| **当前日期** | 2025 年中旬起持续维护 |

---

## 2. 支持的三款 MCU

| MCU | 架构 | 内核 | 构建预设 |
|-----|------|------|----------|
| STM32F103C8T6 (中容量) | ARM | Cortex-M3 | `Debug-F103` |
| STM32F407VET6 | ARM | Cortex-M4 + FPU | `Debug-F407` |
| TI MSPM0G3507 | ARM | Cortex-M0+ | `Debug-G3507` |

三条 MCU 产品线在同一个 CMake 工程中维护，通过 `ENROLL_MCU_TARGET` 宏 (0/1/2) 在编译期切换。

---

## 3. 分层架构总览

```
┌────────────────────────────────────────────┐
│  A_Entry/      程序入口                     │  唯一 main.c
│  app/          应用层                       │  业务逻辑、控制算法、任务调度
│  - Control/    控制逻辑                     │
│  - Control_Task/ 任务调度+中断回调          │
│  - PID/        PID 控制器                   │
│  - Filter/     滤波器                       │
│  - My_Usart/   串口打印管理                 │
└────────────────────────────────────────────┘
              ↓ 调用
┌────────────────────────────────────────────┐
│  BSP/          板级支持层                   │  封装板载器件，提供稳定设备接口
│  - LED/KEY     IO 控制型外设                │
│  - OLED        显示屏 (I2C+SPI 双模式)      │
│  - MPU6050     6 轴传感器 + DMP             │
│  - TB6612      电机驱动                     │
│  - NRF24L01    2.4G 无线模块                │
│  - BMP280      气压传感器                   │
│  - QMC5883P    磁力计                       │
└────────────────────────────────────────────┘
              ↓ 依赖
┌────────────────────────────────────────────┐
│  Enroll/       注册层 (★核心特色)           │  硬件资源注册中心
│  - Enroll.h    对外接口                     │
│  - Enroll.c    注册实现 (X-Macro 展开)      │
│  - Enroll_Internal.h  内部依赖              │
│  - xxx_hw_config.h  板级映射表 (3个MCU)     │
└────────────────────────────────────────────┘
              ↓ 绑定
┌────────────────────────────────────────────┐
│  API/          片内外设抽象接口层            │  统一接口，屏蔽芯片差异
│  inc/ + src/   gpio/adc/pwm/tim/usart/exti  │
│  API_I2C/      I2C 协议层 (平台无关)        │
│  API_SPI/      SPI 协议层 (平台无关)        │
│  通过 soft_xxx_hal 桥接到 Core 层            │
└────────────────────────────────────────────┘
              ↓ 分发
┌────────────────────────────────────────────┐
│  Core/         芯片底层实现                  │  按 MCU 分目录
│  STM32F103/    {src,inc}/f103_*.c,h         │
│  STM32F407/    {src,inc}/f407_*.c,h         │
│  MSPM0G3507/   {src,inc}/G3507_*.c,h        │
│  (每目录还含: sys, delay, cmake/linker)     │
└────────────────────────────────────────────┘
              ↓ 基于
┌────────────────────────────────────────────┐
│  Drivers/      驱动资源层                   │  启动文件、CMSIS/HAL/SDK
│  Drivers_STM32F1/  - std_periph 启动        │
│  Drivers_STM32F4/  - std_periph 启动        │
│  Drivers_M0G3507/  - TI DriverLib + CMSIS   │
└────────────────────────────────────────────┘
              ↓ 基础
┌────────────────────────────────────────────┐
│  SYSTEM/       系统层                       │  系统配置与初始化
│  sys.c/h      系统初始化 / 中断分发         │
│  Delay.h      统一延时接口                  │
│  BusRate.h    软件总线选择+速率集中配置     │
│  IrqPriority.h 统一中断优先级管理           │
└────────────────────────────────────────────┘
```

---

## 4. 核心设计模式

### 4.1 注册层模式 (Enroll — 最重要的设计)

注册层的本质是**用编译期 X-Macro 将"逻辑外设 ID"映射到"物理引脚+硬件实例"**。

**数据流：**
```
xxx_hw_config.h (板级映射宏)
    ↓ 定义 HW_xxx_MAP(X) 宏
Enroll.c (X-Macro 展开)
    ↓ 展开为结构体数组 s_xxxTable[]
Enroll_xxx_Register() (门面函数)
    ↓ 传入结构体数组 + 计数
API/BSP 的 Register() 函数
    ↓ 写入内部管理数组
后续 API/BSP 用逻辑 ID 操作
```

**示例 — G3507 的 LED 注册：**

`G3507_hw_config.h` 定义映射宏：
```c
#define HW_LED_MAP(X) \
    X(LED1, GPIOB, DL_GPIO_PIN_14) \
    X(LED2, GPIOA, DL_GPIO_PIN_29) \
    X(LED3, GPIOA, DL_GPIO_PIN_28) \
    X(Buzzer1, GPIOA, DL_GPIO_PIN_14)
```

`Enroll.c` 用 X-Macro 展开为配置表：
```c
#define ENROLL_LED_ITEM(id, port, pin) \
    { id, port, pin, ENROLL_GPIO_INIT_FN, ENROLL_GPIO_WRITE_FN },
static const LED_Config_t s_ledTable[] = {
    HW_LED_MAP(ENROLL_LED_ITEM)
};
```

优势：切换 MCU 时只需提供新的 `xxx_hw_config.h`，Enroll.c 无需改动。

### 4.2 API 条件编译分发模式

API 层提供统一接口，内部通过 `#if ENROLL_MCU_TARGET` 分发：

```c
// API/inc/gpio.h — 接口声明
void API_GPIO_Write(void *port, uint32_t pin, uint8_t level);

// API/src/gpio.c — 实现分发
void API_GPIO_Write(void *port, uint32_t pin, uint8_t level) {
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
    F103_GPIO_Write(port, pin, level);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
    F407_GPIO_Write(port, pin, level);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
    G3507_GPIO_Write(port, pin, level);
#endif
}
```

**注意：** 这是编译期多态，不是运行时虚表 — 一个固件只编译一个 MCU 目标。

### 4.3 两阶段初始化模式 (Register → Init)

外设资源使用两阶段初始化：

```
Enroll_xxx_Register()  // 阶段1: 登记配置表 (填充内部数组)
        ↓
API_xxx_Init(id, ...)  // 阶段2: 激活硬件 (写寄存器、开启时钟)
```

这在 `main.c` 中体现得最明显：
```c
Enroll_USART_Register();           // 先登记 USART 配置表
API_USART_Init(API_USART1, 115200); // 再用逻辑 ID 初始化特定 USART
```

### 4.4 void *port 跨 MCU 抽象

由于不同 MCU 的 GPIO 端口类型不同：
- STM32: `GPIO_TypeDef *` (如 `GPIOA`)
- MSPM0: `GPIO_Regs *` (如 `GPIOA`)

API 层统一用 `void *port` 传递端口指针，在 Core 层内部转回实际类型：
```c
gpioPort = (GPIO_Regs *)port;  // G3507 Core
// 或
gpioPort = (GPIO_TypeDef *)port; // F103 Core
```

### 4.5 软件总线 (bit-bang I2C/SPI) — 双分层架构

项目目前使用软件模拟的 I2C 和 SPI（非硬件外设），原因：
- 灵活性高，不受硬件 I2C/SPI 实例限制
- 跨平台一致性好（软件模拟行为相同）
- 引脚映射自由

**V3.1 架构升级**：将原先混在一起的 `app/My_I2c/` 和 `app/My_SPI/` 拆为两层：

```
API/API_I2C/API_I2C.c          ← 协议逻辑 (平台无关, 始终编译)
    │  Start/Stop/SendByte/ReceiveByte/Wait_Ack/...
    │  通过 soft_i2c_hal.h 桥接 ↓
    │
Core/{platform}/{platform}_soft_i2c.c  ← GPIO 翻转+延时 (CMake 按平台选一个)
    │  直接寄存器访问: BSRR/BRR/DOUTSET/DOUTCLR/IOMUX...
```

```
API/API_SPI/API_SPI.c          ← 协议逻辑 (Start/Stop/SwapByte)
    │  通过 soft_spi_hal.h 桥接 ↓
Core/{platform}/{platform}_soft_spi.c  ← GPIO 翻转+延时
```

**HAL 桥接接口** (`soft_i2c_hal.h`, `soft_spi_hal.h`)：
- **内部桥接**：API 协议层 ↔ Core 底层实现之间的桥梁
- **不对外暴露**：BSP/App 层不应直接引用，统一通过 `API_I2C.h` / `API_SPI.h` 操作
- 声明平台无关的底层原语函数（W_SCL, W_SDA, R_SDA, W_CS, W_SCK, delay_us 等）
- 由 Core 层各平台各自实现（零 `#if`，直接寄存器操作）

**设计原则**：
- **所有 BSP 设备**只通过 `API_I2C.h` / `API_SPI.h` 的标准协议函数操作总线
- 总线选择+速率集中配置在 [SYSTEM/BusRate.h](SYSTEM/BusRate.h)，新增设备只需加两行宏

### 4.6 中断优先级统一管理 (IrqPriority.h)

对标 `BusRate.h` 的思路，[SYSTEM/IrqPriority.h](SYSTEM/IrqPriority.h) 集中管理所有 NVIC 中断优先级：

```
IrqPriority.h (策略层)  →  API/Core (机制层)  →  NVIC 硬件寄存器
  "谁比谁高"                   "怎么设"              "硬件执行"
```

**优先级分配 (当前)**：
| 优先级 | 中断源 | 理由 |
|:---:|--------|------|
| 0 | SysTick | 系统心跳 |
| 1 | API_TIM | 1ms 控制节拍，所有 PID 回路的心脏 |
| 2 | MPU6050 EXTI | 姿态数据，串级控制外环输入，实时性高于速度环 |
| 3 | 编码器 EXTI | 速度内环反馈 |
| 4 | USART ×3 | 通信（丢包可重传） |

**多 MCU 适配**：
- STM32F103/F407：Cortex-M3/M4，4bit NVIC → 0~15 级，每级独立
- MSPM0G3507：Cortex-M0+，2bit NVIC → 0~3 级，需压缩（MPU6050 与 Encoder 同级）

**关键设计**：
- `IRQ_PRIO` 宏 = 抢占优先级（数字越小越高，高优先级 ISR 可打断低优先级）
- `IRQ_SUB` 宏 = 响应优先级/子优先级（仅同抢占优先级的中断同时到达时决定顺序，当前未使用填 0）
- PID 计算本身在 main loop 中执行，不是 ISR，无需 NVIC 优先级
- 改优先级只需改一行宏，所有平台自动生效

**调用链（以 MPU6050 为例）**：
```
IrqPriority.h: #define IRQ_PRIO_MPU6050 2U
    → Enroll.c: API_EXTI_Init(id, trigger, IRQ_PRIO_MPU6050, IRQ_SUB_PRIO_MPU6050)
    → API/exti.c: API_EXTI_CoreInit(port, pin, ..., preemptPriority=2, subPriority=0)
    → Core/exti.c: NVIC_SetPriority(irqn, 2) — 写入硬件寄存器
```

---

## 5. 当前 API 层支持的外设接口

| API 头文件 | 功能 | G3507 支持 | F103 支持 | F407 支持 |
|-----------|------|:---:|:---:|:---:|
| [gpio.h](API/inc/gpio.h) | GPIO 输入/输出 | ✅ | ✅ | ✅ |
| [usart.h](API/inc/usart.h) | 串口通信 | ✅ | ✅ | ✅ |
| [pwm.h](API/inc/pwm.h) | PWM 输出 | ✅ | ✅ | ✅ |
| [tim.h](API/inc/tim.h) | 定时器中断 | ✅ | ✅ | ✅ |
| [adc.h](API/inc/adc.h) | ADC 采集 | ✅ | ✅ | ✅ |
| [exti.h](API/inc/exti.h) | 外部中断 | ✅ | ✅ | ✅ |
| [Encoder.h](API/inc/Encoder.h) | 编码器接口 | ✅ (EXTI) | ✅ (TIM 模式) | ✅ (TIM 模式) |
| [API_I2C.h](API/API_I2C/API_I2C.h) | 软件 I2C 协议 (平台无关) | ✅ (soft_i2c_hal) | ✅ (soft_i2c_hal) | ✅ (soft_i2c_hal) |
| [API_SPI.h](API/API_SPI/API_SPI.h) | 软件 SPI 协议 (平台无关) | ✅ (soft_spi_hal) | ✅ (soft_spi_hal) | ✅ (soft_spi_hal) |

**说明**：
- **EXTI**：三平台均已完整实现。F103 用 AFIO+EXTI 寄存器，F407 用 SYSCFG+EXTI，G3507 用 DL_GPIO TI DriverLib。
- **Encoder**：F103/F407 使用定时器硬件编码器模式（无需中断），G3507 使用外部中断软件模拟。
  Core 层已通用化，所有 port/pin 参数化——改 `hw_config.h` 即可换引脚，无需改 Core 代码。
- **I2C/SPI**：V3.1 采用双分层架构——API 层负责协议逻辑（平台无关，始终编译），Core 层负责 GPIO 翻转+延时（CMake 按平台选一个编译）。
  中间通过 `soft_i2c_hal.h` / `soft_spi_hal.h` 桥接（内部接口，BSP 不接触）。

---

## 6. BSP 层当前支持的器件

| 模块 | 文件 | 接口类型 | 专用性 |
|------|------|---------|:---:|
| LED | [LED.c](BSP/LED/LED.c) | GPIO 输出 | 三平台通用 |
| KEY | [KEY.c](BSP/KEY/KEY.c) | GPIO 输入 (消抖) | 三平台通用 |
| OLED | [OLED.c](BSP/OLED/OLED.c) | SPI / I2C 双模式 | 三平台通用 |
| MPU6050 | [MPU6050.c](BSP/MPU6050/MPU6050.c) | I2C + 外部中断 | 三平台通用 |
| MPU6050 DMP | [eMPL/](BSP/MPU6050/eMPL/) | InvenSense 官方 DMP 库 | 三平台通用 |
| TB6612 | [TB6612.c](BSP/TB6612/TB6612.c) | PWM + GPIO | 三平台通用 |
| NRF24L01 | [NRF24L01.c](BSP/NRF24L01/NRF24L01.c) | SPI | F103 + F407 |
| BMP280 | [BMP280.c](BSP/BMP280/BMP280.c) | I2C | 仅 F407 |
| QMC5883P | [QMC5883P.c](BSP/QMC5883P/QMC5883P.c) | I2C | 仅 F407 |

---

## 7. SYSTEM 层与 Core exti/sys 文件角色说明

### 为什么 103/407 只有 `exti.h` 和 `sys.c`？

这是历史命名遗留问题，容易误导：

| MCU | 文件 | 实际作用 | 为什么 |
|-----|------|---------|--------|
| **F103** | `src/f103_exti.c` | EXTI 初始化实现（AFIO+EXTI+NVIC） | 2025 年修正：之前误命名为 f103_sys.c |
| | `inc/f103_exti.h` | EXTI 函数声明 | |
| | 无时钟 sys | 不需要 — STM32 启动时 `SystemInit()` 已配好时钟 |
| **F407** | `src/f407_exti.c` | EXTI 初始化实现（SYSCFG+EXTI+NVIC） | 同 F103 模式 |
| | `inc/f407_exti.h` | EXTI 函数声明 | 唯一的头文件 ✓ |
| | 无时钟 sys | 不需要 — STM32 启动时 `SystemInit()` 已配好时钟 |
| **G3507** | `G3507_sys.c/h` | **时钟初始化**（80MHz PLL）+ 系统信息查询 | G3507 需要自定义 PLL 配置 |
| | `src/G3507_exti.c` + `inc/G3507_exti.h` | **EXTI 初始化**（DL_GPIO + NVIC） | G3507 GPIOn 高位引脚(PIN24-31)比 STM32 更复杂 |

**关键区别**：
- STM32 的 `SystemInit()` 在启动文件中由 CMSIS 标准库自动调用，所以 Core 层不需要 sys clock 代码
- G3507 的时钟策略（PLL 80MHz）需要自定义初始化，所以要 `G3507_sys.c/h`
- F103 和 F407 的 `xxx_sys.c` 文件名有误导性——它们其实应该叫 `xxx_exti.c`

### SYSTEM 层 vs Core 层分工

```
SYSTEM/sys.c                          ← 平台无关的门面层
  ├─ SYS_Init()                       → 条件编译分发到 Core 时钟初始化
  │   └─ F103: 空（SystemInit 已做）  → 无额外操作
  │   └─ F407: 空（SystemInit 已做）  → 无额外操作
  │   └─ G3507: G3507_SYS_Init()      → Core 层配 80MHz PLL
  ├─ SYS_EXTI_GetIrqn(port, pin)      → 端口+引脚 → NVIC IRQ 号
  │   └─ F103/407: 引脚线号 → EXTIn_IRQn
  │   └─ G3507: 端口 → GPIOA/B_INT_IRQn
  └─ SYS_EXTI_GetLineIndex(pin)       → 引脚掩码 → 0~15 线号（三平台通用）

Core/STM32F103/src/f103_exti.c         ← EXTI 硬寄存器实现（F103 AFIO 体系）
Core/STM32F407/src/f407_exti.c         ← EXTI 硬寄存器实现（F407 SYSCFG 体系）
Core/MSPM0G3507/G3507_sys.c           ← 系统时钟初始化（80MHz PLL）
Core/MSPM0G3507/src/G3507_exti.c      ← EXTI TI DriverLib 实现
```

---

## 8. 关键文件索引

### 构建系统
| 文件 | 作用 |
|------|------|
| [CMakeLists.txt](CMakeLists.txt) | 统一构建入口，含平台分支逻辑 |
| [CMakePresets.json](CMakePresets.json) | CMake 预设 (Debug/F103/F407/G3507) |
| [gcc-arm-none-eabi.cmake](gcc-arm-none-eabi.cmake) | ARM GCC 交叉编译工具链 |
| [.vscode/tasks.json](.vscode/tasks.json) | VS Code 构建/烧录任务 |
| [.vscode/settings.json](.vscode/settings.json) | VS Code CMake 源目录配置 |
| [.vscode/set-default-mcu-target.ps1](.vscode/set-default-mcu-target.ps1) | 切换 Enroll.h 默认 MCU 目标的脚本 |

### 注册层 (最需要关注的目录)
| 文件 | 作用 |
|------|------|
| [Enroll/Enroll.h](Enroll/Enroll.h) | 注册层对外接口 + ENROLL_MCU_TARGET 默认定义 |
| [Enroll/Enroll.c](Enroll/Enroll.c) | X-Macro 展开 + Register 门面函数 |
| [Enroll/Enroll_Internal.h](Enroll/Enroll_Internal.h) | 仅 Enroll.c 使用的内部依赖 |
| [Enroll/103_hw_config.h](Enroll/103_hw_config.h) | F103 板级引脚映射 |
| [Enroll/407_hw_config.h](Enroll/407_hw_config.h) | F407 板级引脚映射 |
| [Enroll/G3507_hw_config.h](Enroll/G3507_hw_config.h) | G3507 板级引脚映射 |
| [Enroll/G3507_pinmux.h](Enroll/G3507_pinmux.h) | G3507 IOMUX 引脚索引表 |

### SYSTEM 层与 Core exti/sys 文件
| 文件 | 实际作用 |
|------|---------|
| [SYSTEM/sys.c](SYSTEM/sys.c) | SYS_Init/EXTI_GetIrqn/LineIndex 门面（条件编译分发） |
| [SYSTEM/sys.h](SYSTEM/sys.h) | 系统初始化和 EXTI 辅助接口声明 |
| [SYSTEM/Delay.h](SYSTEM/Delay.h) | 统一延时接口（Delay_us/ms/s） |
| [SYSTEM/BusRate.h](SYSTEM/BusRate.h) | 软件总线选择+速率集中配置 |
| [SYSTEM/IrqPriority.h](SYSTEM/IrqPriority.h) | NVIC 中断优先级统一管理 |
| [Core/STM32F103/src/f103_exti.c](Core/STM32F103/src/f103_exti.c) | F103 EXTI 实现（AFIO+EXTI+NVIC） |
| [Core/STM32F407/src/f407_exti.c](Core/STM32F407/src/f407_exti.c) | F407 EXTI 实现（SYSCFG+EXTI+NVIC） |
| [Core/MSPM0G3507/G3507_sys.c](Core/MSPM0G3507/G3507_sys.c) | G3507 时钟初始化（80MHz PLL） |
| [Core/MSPM0G3507/src/G3507_exti.c](Core/MSPM0G3507/src/G3507_exti.c) | G3507 EXTI 实现（DL_GPIO + NVIC） |

### 应用层核心
| 文件 | 作用 |
|------|------|
| [A_Entry/main.c](A_Entry/main.c) | 程序入口，完整的初始化流程 + 主循环 |
| [app/Control_Task/](app/Control_Task/) | 控制任务调度 + 中断回调 |
| [app/PID/](app/PID/) | PID 控制器实现 |
| [app/Filter/](app/Filter/) | 滤波器实现 |
| [API/API_I2C/](API/API_I2C/) | 软件 I2C 协议层 (平台无关) |
| [API/API_SPI/](API/API_SPI/) | 软件 SPI 协议层 (平台无关) |
| [Core/STM32F103/f103_soft_i2c/](Core/STM32F103/f103_soft_i2c/) | F103 I2C GPIO 翻转+延时 |
| [Core/STM32F103/f103_soft_spi/](Core/STM32F103/f103_soft_spi/) | F103 SPI GPIO 翻转+延时 |
| [Core/STM32F407/f407_soft_i2c/](Core/STM32F407/f407_soft_i2c/) | F407 I2C GPIO 翻转+延时 |
| [Core/STM32F407/f407_soft_spi/](Core/STM32F407/f407_soft_spi/) | F407 SPI GPIO 翻转+延时 |
| [Core/MSPM0G3507/G3507_soft_i2c/](Core/MSPM0G3507/G3507_soft_i2c/) | G3507 I2C GPIO 翻转+延时 |
| [Core/MSPM0G3507/G3507_soft_spi/](Core/MSPM0G3507/G3507_soft_spi/) | G3507 SPI GPIO 翻转+延时 |
| [app/My_Usart/](app/My_Usart/) | 串口打印封装 |

---

## 9. 开发约定与命名规范

### 函数命名
- `API_xxx_*` — API 层对外接口 (如 `API_GPIO_Write`)
- `F103_xxx_*` — F103 Core 层实现
- `F407_xxx_*` — F407 Core 层实现
- `G3507_xxx_*` — G3507 Core 层实现
- `Enroll_xxx_*` — 注册层门面函数
- `API_I2C_*` / `API_SPI_*` — 软件总线协议层
- `soft_i2c_hal_*` / `soft_spi_hal_*` — 总线 HAL 桥接接口 (由 Core 层实现)

### 文件组织
- 每个外设模块在自己的目录下，含 `.c` + `.h`
- API 层: `inc/` 放头文件，`src/` 放实现
- Core 层: `inc/` 放头文件，`src/` 放实现，`cmake/` 放链接脚本
- BSP 模块: 头文件与源文件平级放在模块目录

### 条件编译
- 统一通过 `ENROLL_MCU_TARGET` 宏分发，不引入额外的 feature flag
- `ENROLL_MCU_TARGET` 默认值定义在 [Enroll/Enroll.h](Enroll/Enroll.h) 中

### 平台差异处理
- 通用代码放各层通用列表
- 平台差异代码在各 MCU 分支中独立维护
- BSP 器件按"通用列表 + 平台追加"组织

---

## 10. 新增 MCU 的接入步骤

参照 README 描述，实际工程中的接入流程：

1. **Drivers 层** — 在 `Drivers/` 下新增 `Drivers_NewMCU/` 目录，放入启动文件 + SDK/HAL
2. **Core 层** — 在 `Core/NewMCU/` 下实现 `NewMCU_gpio/adc/pwm/tim/usart/...` 等底层驱动
3. **Enroll 层** — 在 `Enroll/` 下新增 `NewMCU_hw_config.h`，定义所有板级映射宏
4. **CMakeLists.txt** — 新增 `elseif(RESOLVED_MCU_TARGET STREQUAL "NewMCU")` 分支
5. **CMakePresets.json** — 新增 `Debug-NewMCU` 构建预设
6. **OpenOCD** — 新增对应的 `.cfg` 下载配置
7. **SYSTEM/BusRate.h** — 新增该 MCU 的总线选择+速率配置

---

## 11. 当前工程状态

### 已完成 (近期)
- **I2C/SPI 架构重构 (V3.1)**：将 `app/My_I2c/` 和 `app/My_SPI/` 拆为 API 协议层 + Core 底层双层架构
  - API 层：`API/API_I2C/` + `API/API_SPI/`，平台无关协议逻辑，始终编译
  - Core 层：每 MCU 独立的 `{platform}_soft_i2c.c` + `{platform}_soft_spi.c`，CMake 按平台选一个
  - HAL 桥接：`soft_i2c_hal.h` / `soft_spi_hal.h` 为 API↔Core 内部桥梁，BSP/App 不接触
  - 消除了原先 `My_I2C.c` 和 `My_SPI.c` 中大量的 `#if ENROLL_MCU_TARGET` 块
- **BSP 统一到 API**：OLED/NRF24L01 等所有 BSP 设备只通过 `API_I2C.h` / `API_SPI.h` 操作总线
- **OLED 驱动重构**：基于江协科技参考代码重写，I2C 模式走标准协议（SendByte + Wait_Ack）
- **配置集中化**：总线选择+速率收至 `SYSTEM/BusRate.h`，中断优先级收至 `SYSTEM/IrqPriority.h`
- **文件迁移**：`main.c` → `A_Entry/`，`BusRate.h` `IrqPriority.h` → `SYSTEM/`
- **文档**：`docs/architecture-guide.md` → `docs/arch-guide.md`
- **编码器 (Encoder)**：三平台完整实现
  - F103/F407：定时器硬件编码器模式，通用 port/pin 参数化（改 hw_config 即生效）
  - G3507：外部中断模拟，上升沿触发 + 读另一相电平判方向
  - API 层：`API_Encoder_Register/Init/GetSpeed`，20ms 周期读取
- **速度环 PID**：基于现有 PID 库完成左右轮速度闭环控制
  - `PID_EncoderSpeed_t` 内部左右独立 `PID_TypeDef`，共用 kp/ki/kd
  - **关键经验**：ki 值需要乘以 `1/dt` 倍补偿 dt 因子（库中 error_sum 乘以 dt），否则 I 项积累过慢
  - 电机系统通常不需要 D 项（kd=0），微分噪声放大导致卡顿
  - `Out_max` 必须匹配 TB6612_MAX_DUTY（400），否则反积分饱和失效
- **中断优先级统一管理**：[SYSTEM/IrqPriority.h](SYSTEM/IrqPriority.h)
  - 集中定义抢占/响应优先级宏，三平台自动适配 NVIC 位宽差异
  - 填补了 6 个 Core 文件"只使能中断不设优先级"的空白
  - 修复 G3507 编码器 IRQn 错误（两个编码器都用了 GPIOA_INT_IRQn）
- TB6612 电机驱动三平台通用化
- KEY 驱动修复多路返回值，优化消抖逻辑

### 注意事项
- FreeRTOS-LTS、USB 协议库、TI 官方 SDK 源文件不再同步上传至 GitHub
- 工程保留 MDK_ARM 目录用于 Keil IDE 兼容
- G3507 构建通过 TI DriverLib 实现，区别于 STM32 的标准外设库
- D 项（kd）在直流电机速度环中容易放大编码器量化噪声导致卡顿，通常设为 0

---

## 12. 快速上手检查清单

为新对话恢复认知时，按以下顺序阅读关键文件：

1. ✅ 本文档 (docs/arch-guide.md) — 架构全貌
2. [README.md](README.md) — 项目简介与构建命令
3. [CMakeLists.txt](CMakeLists.txt) — 理解源文件组织与平台分支
4. [Enroll/Enroll.h](Enroll/Enroll.h) — 注册层接口全览
5. [Enroll/G3507_hw_config.h](Enroll/G3507_hw_config.h) — 当前默认 MCU 的板级映射
6. [A_Entry/main.c](A_Entry/main.c) — 典型的初始化流程
7. [SYSTEM/IrqPriority.h](SYSTEM/IrqPriority.h) — 中断优先级策略
8. [SYSTEM/BusRate.h](SYSTEM/BusRate.h) — 软件总线速率策略
9. 任一 `API/src/*.c` — 理解 API→Core 分发模式
10. 任一 `Core/*/src/*.c` — 理解 Core 层实现风格

# OmniLayer - 分层不是目的，隔离变化才是

一个面向多 MCU 与多开发范式的嵌入式工程分层开发架构框架，使用 CMake + GCC + OpenOCD 统一构建、烧录和维护流程。

当前支持/覆盖方向：STM32F103、STM32F407、TI MSPM0G3507。

## 🚀 项目定位

OmniLayer 的核心目标是把应用逻辑和芯片实现解耦，让工程可迁移、可扩展、可长期维护。
    
- 同一套业务代码，按目标芯片切换底层实现。
- 统一构建入口，减少多工程并行维护成本。
- VS Code/Trae 工作流，保留keil，兼容不同团队开发习惯。

## 🌿 分支策略

为了让协作者快速理解仓库方向，当前分支职责如下：

| 分支 | 定位 | 状态 |
|---|---|---|
| `main` | 裸机开发主线（当前稳定主架构） | 持续维护 |
| `FreeRTOS` | 操作系统开发主线（RTOS 方向） | 持续维护 |

## ✨ 架构亮点

- 🧭 多目标工程：同一仓库支持多 MCU 目标扩展。
- 🧱 分层清晰：应用层、接口层、BSP、注册层(核心思想)、核心层职责明确。
- ⚙️ 工具统一：CMakePresets 构建，OpenOCD 烧录，流程一致。
- 🚌 软件总线：I2C/SPI 参数集中配置，调优成本低。
- 🔁 双 IDE 兼容：VS Code/Trae + CMake 与 Keil 并行可用。

## 🧩 注册层（Enroll）特色

`Enroll/` 是本项目最有辨识度的一层，作用可以理解为“硬件资源注册中心”：

- 把板级外设资源映射到具体 MCU 引脚与外设实例。
- 让上层 BSP / APP 不需要关心不同芯片的引脚差异。
- 切换 MCU 时，主要改注册与底层映射，不重写整套业务逻辑。

## 📁 项目结构

```text
OmniLayer/
├─ A_Entry/                    # 程序入口 (main.c)
│  └─ main.c
├─ API/                        # MCU 片内外设抽象接口层
│  ├─ inc/                     # gpio/adc/pwm/tim/usart/exti 等
│  ├─ src/                     # API 实现 (条件编译分发到 Core)
│  ├─ API_I2C/                 # I2C 协议层
│  └─ API_SPI/                 # SPI 协议层
├─ app/                        # 应用层：业务逻辑、控制算法
│  ├─ Control/
│  ├─ Control_Task/
│  ├─ Filter/                  # 滤波器
│  ├─ PID/                     # PID 控制器
│  └─ My_Usart/                # 串口数据函数管理
├─ BSP/                        # 板级支持层：OLED/MPU6050/QMC5883P/NRF24L01 等
│  ├─ BMP280/
│  ├─ KEY/
│  ├─ LED/
│  ├─ MPU6050/
│  ├─ NRF24L01/
│  ├─ OLED/
│  ├─ QMC5883P/
│  └─ TB6612/
├─ Core/                       # 芯片相关底层实现（按 MCU 分目录）
│  ├─ STM32F103/               # f103_soft_i2c, f103_soft_spi, src/, inc/
│  ├─ STM32F407/               # f407_soft_i2c, f407_soft_spi, src/, inc/
│  └─ MSPM0G3507/              # G3507_soft_i2c, G3507_soft_spi, src/, inc/
├─ Drivers/                    # 启动文件、SDK/CMSIS 等底层资源
│  ├─ Drivers_STM32F1/
│  ├─ Drivers_STM32F4/
│  └─ Drivers_MSPM0G3507/
├─ Enroll/                     # 硬件资源注册与板级映射（103/407/G3507_hw_config）
├─ Middlewares/                # 中间件（FreeRTOS、USB协议 等）
├─ OpenOCD/                    # 下载配置（F103/F407/G3507）
├─ SYSTEM/                     # 系统级配置与初始化
│  ├─ sys.c sys.h              # 系统初始化 / 中断分发
│  ├─ Delay.h                  # 统一延时接口
│  ├─ BusRate.h                # 软件总线速率+总线选择集中配置
│  └─ IrqPriority.h            # 统一中断优先级管理
├─ MDK_ARM/                    # Keil 工程（保留兼容开发习惯）
├─ build/                      # 构建输出目录（Debug/F103/F407/G3507...）
├─ CMakeLists.txt              # 统一构建入口
├─ CMakePresets.json           # 构建预设
└─ gcc-arm-none-eabi.cmake     # GCC ARM 交叉编译工具链
```

## 🏗️ 分层说明

| 层级 | 目录 | 职责 |
|---|---|---|
| 入口层 | `A_Entry/` | 唯一 main.c 程序入口 |
| 应用层 | `app/` | 控制任务、业务逻辑、算法组合 |
| 接口层 | `API/` | 统一片内外设接口 + I2C/SPI 协议层，屏蔽芯片差异 |
| 板级层 | `BSP/` | 封装板载器件，向上提供稳定设备接口 |
| 注册层 | `Enroll/` | 资源映射与注册，衔接板级与芯片层 |
| 核心层 | `Core/` | GPIO/TIM/USART/I2C-GPIO/SPI-GPIO 等 MCU 相关实现 |
| 系统层 | `SYSTEM/` | 时钟、系统初始化、中断分发 |
| 驱动资源层 | `Drivers/` | 启动文件、CMSIS/HAL/标准库 |
| 中间件层 | `Middlewares/` | FreeRTOS、USB 等可复用组件 |

## ⚙️ 构建与烧录

### ⌨️ VS Code 快捷键

- `F7`：编译（先配置再构建，对应 Build Debug）
- `F8`：烧录（先编译再烧录，对应 Flash Debug）
- `Ctrl+Shift+F1`：弹窗选择 MCU 后编译（`Build Select MCU`）
- `Ctrl+Shift+F2`：弹窗选择 MCU 后下载（`Flash Select MCU`）
- `Ctrl+Shift+F3`：弹窗选择并写入默认 MCU 宏（`Set Default MCU Target`）

说明：
`F7`/`F8` 仍走默认 `Debug` 预设，并沿用 `ENROLL_MCU_TARGET` 默认宏对应目标；
`Ctrl+Shift+F1`/`Ctrl+Shift+F2` 走 `Debug-F103` / `Debug-F407` / `Debug-G3507` 选择式流程，无需手工改 `Enroll.h` 中的 `ENROLL_MCU_TARGET`。
`Ctrl+Shift+F3` 会把 `Enroll.h` 里的 `ENROLL_MCU_TARGET` 更新为所选目标，之后可直接用 `F7`/`F8` 走默认流程编译和下载。

### 🔧 命令行方式

```bash
cmake --preset Debug
cmake --build --preset Debug
```

### 🛰️ OpenOCD 配置

- `OpenOCD/F103_OpenOCD.cfg`
- `OpenOCD/F407_OpenOCD.cfg`
- `OpenOCD/G3507_OpenOCD.cfg`

## 🚌 软件总线（I2C / SPI）

- 采用 **API 协议 + Core 底层** 双分层架构：
- 总线速率和总线选择集中在 `SYSTEM/BusRate.h` 配置，新增设备/调速/换总线只需改一个文件。

## 🎯 中断优先级统一管理

- 提供统一的中断向量优先级配置策略
- 改优先级只需改一行宏，所有平台自动生效。

- **策略文件**：[`SYSTEM/IrqPriority.h`] — 集中定义所有中断的抢占/响应优先级宏。
- **原则**：数字越小优先级越高，Core 层只提供"怎么设"的机制，`IrqPriority.h` 决定"谁比谁高"的策略。
- **多平台适配**：STM32F103/F407（Cortex-M3/M4, 4bit NVIC - 0~15）和 MSPM0G3507（Cortex-M0+, 2bit NVIC - 0~3）自动按宏展开正确数值。

## 🧱 新增 MCU 快速接入

1. 在 `Core/`、`SYSTEM/`、`Drivers/Start/` 补齐该芯片最小启动与系统文件。
2. 在根 `CMakeLists.txt` 新增 MCU 分支。
3. 在 `OpenOCD/` 新增对应下载配置（建议命名 `Fxxx_OpenOCD.cfg`）。
4. 在 `Enroll/xxx_hw_config.h` 补齐板级映射。
5. 复用现有构建/烧录流程，快速落地新目标。

## 📌 维护原则

- 业务逻辑尽量不直接操作寄存器。
- 目标相关代码集中在 Core / SYSTEM / Drivers。
- BSP 接口尽量稳定，切换芯片时优先替换映射与底层实现。

## 📖 详细说明

1. 完整工程架构文档：docs/arch-guide.md，作为 README 的细化补充说明。
2. 该文档定向用于 AI 上下文初始化，新建对话场景下，直接加载此文档就能让 AI 快速熟悉项目分层、设计规范与工程逻辑。

## ⚠️ 注意事项

1. FreeRTOS-LTS、USB 协议、TI 官方 SDK 相关源码不再同步上传至本项目 GitHub 仓库，如需使用上述资源，请开发者前往各产品官方网站自行获取。
2. 受开发精力限制，项目主力维护 VS Code + CMake 编译环境；Keil MDK 配套工程无法同步迭代更新，如需使用 Keil 编译，需使用者自行手动补齐缺失头文件与工程配置。

## 🧠 个人架构设计感悟

### 1. 封装的代价是真实的

架构越通用，分层越多，函数调用链就越长。对于 I2C/SPI 这种"每 bit 都是热路径"的外设，多一层函数调用就是数量级的性能损失。之前为了"屏蔽 MCU + 软件模拟和硬件实现差异"，把 I2C 包了 5~6 层，结果**架构版的 I2C 还跑不过 Keil 标准库的版本**。

### 2. 架构不是目的，是手段

封装是为解决问题服务的，不要为了"看起来优雅"而过度设计。**性能不行的架构，再漂亮也是花瓶。**

### 3. 正确的分层策略：按速度分治

| 设备类型 | 举例 | 策略 |
|----------|------|------|
| 慢速外设 | LED、KEY、UART | 享受架构红利，走完整 API 分发链 |
| 快速外设 | I2C、SPI | 拆成两层——协议逻辑放 API 层（复用），GPIO 翻转放 Core 层（零开销） |

### 4. 架构的核心价值

架构的价值在于**隔离变化**——MCU 变了只换 Core 层，业务逻辑纹丝不动。当你发现自己在为"通用性"牺牲性能时，就该停下来重新审视分层边界了。

> **一句话总结**：架构的价值在于解决实际问题，而不是展示设计模式。当通用性和性能冲突时，性能优先。

## 📮 项目状态与联系

- 项目持续维护中。
- 如果你在使用过程中遇到问题，欢迎联系：
- QQ 邮箱：634591772@qq.com

# OmniLayer

一个面向多 MCU 与多开发范式的嵌入式工程框架，使用 CMake + GCC + OpenOCD 统一构建、烧录和维护流程。

当前支持/覆盖方向：STM32F103、STM32F407，以及 TI MSPM0G3507。

## 🚀 项目定位

OmniLayer 的核心目标是把应用逻辑和芯片实现解耦，让工程可迁移、可扩展、可长期维护。

- 同一套业务代码，按目标芯片切换底层实现。
- 统一构建入口，减少多工程并行维护成本。
- 保留 VS Code 与 Keil 双工作流，兼容不同团队习惯。

## 🌿 分支策略

为了让协作者快速理解仓库方向，当前分支职责如下：

| 分支 | 定位 | 状态 |
|---|---|---|
| `main` | 裸机开发主线（当前稳定主架构） | 持续维护 |
| `FreeRTOS` | 操作系统开发主线（RTOS 方向） | 持续维护 |

## ✨ 架构亮点

- 🧭 多目标工程：同一仓库支持多 MCU 目标扩展。
- 🧱 分层清晰：应用层、接口层、BSP、注册层、核心层职责明确。
- ⚙️ 工具统一：CMakePresets 构建，OpenOCD 烧录，流程一致。
- 🚌 软件总线：I2C/SPI 参数集中配置，调优成本低。
- 🔁 双 IDE 兼容：VS Code + CMake 与 Keil 并行可用。

## 🧩 注册层（Enroll）特色

`Enroll/` 是本项目最有辨识度的一层，作用可以理解为“硬件资源注册中心”：

- 把板级外设资源映射到具体 MCU 引脚与外设实例。
- 让上层 BSP / APP 不需要关心不同芯片的引脚差异。
- 切换 MCU 时，主要改注册与底层映射，不重写整套业务逻辑。

## 📁 项目结构

```text
OmniLayer/
├─ API/                        # MCU 片内外设抽象接口层（adc/gpio/pwm/tim/usart）
│  ├─ inc/
│  └─ src/
├─ app/                        # 应用层：业务逻辑、控制算法、任务入口
│  ├─ main.c
│  ├─ BusRate.h                # 软件总线速率集中配置
│  ├─ Control/
│  ├─ Control_Task/
│  ├─ Filter/
│  ├─ PID/
│  ├─ My_I2c/
│  ├─ My_SPI/
│  └─ My_Usart/
├─ BSP/                        # 板级支持层：OLED/MPU6050/QMC5883P/NRF24L01 等
│  ├─ BMP280/
│  ├─ KEY/
│  ├─ LED/
│  ├─ MPU6050/
│  ├─ NRF24L01/
│  ├─ OLED/
│  └─ QMC5883P/
├─ Core/                       # 芯片相关底层实现（按 MCU 分目录）
│  ├─ STM32F103/
│  ├─ STM32F407/
│  └─ MSPM0G3507/
├─ Drivers/                    # 启动文件、SDK/CMSIS/HAL 等底层资源
│  ├─ Drivers_STM32F1/
│  ├─ Drivers_STM32F4/
│  └─ Drivers_MSPM0G3507/
├─ Enroll/                     # 硬件资源注册与板级映射（103/407/G3507_hw_config）
├─ Middlewares/                # 中间件（FreeRTOS、USB 等）
├─ OpenOCD/                    # 下载配置（F103/F407/G3507）
├─ SYSTEM/                     # 系统级初始化（时钟/中断/系统配置）
├─ MDK_ARM/                    # Keil 工程（保留兼容开发习惯）
├─ build/                      # 构建输出目录（Debug/F103/F407...）
├─ CMakeLists.txt              # 统一构建入口
├─ CMakePresets.json           # 构建预设
└─ gcc-arm-none-eabi.cmake     # GCC ARM 交叉编译工具链
```

## 🏗️ 分层说明

| 层级 | 目录 | 职责 |
|---|---|---|
| 应用层 | `app/` | 控制任务、业务逻辑、算法组合 |
| 接口层 | `API/` | 统一片内外设接口，屏蔽芯片差异 |
| 板级层 | `BSP/` | 封装板载器件，向上提供稳定设备接口 |
| 注册层 | `Enroll/` | 资源映射与注册，衔接板级与芯片层 |
| 核心层 | `Core/` | GPIO/TIM/USART/中断等 MCU 相关实现 |
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
`Ctrl+Shift+F3` 会把 `Enroll.h` 里的 `ENROLL_MCU_TARGET` 更新为所选目标，之后你可直接用 `F7`/`F8` 走默认流程编译和下载。

### 🔧 命令行方式

```bash
cmake --preset Debug
cmake --build --preset Debug
```

### 🛰️ OpenOCD 配置

- `OpenOCD/F103_OpenOCD.cfg`
- `OpenOCD/F407_OpenOCD.cfg`
- `OpenOCD/G3507_OpenOCD.cfg`

## 🆕 MSPM0G3507 兼容进展

- 已新增 TI MSPM0G3507 的 Core 层适配（GPIO/TIM/USART 等）。
- 已加入 G3507 专用构建预设与烧录链路（CMake + OpenOCD）。
- 已统一串口/定时器的注册分发机制，使多平台行为一致。

## 🚌 软件总线（I2C / SPI）

- 支持软件 I2C 与软件 SPI 的统一管理。
- 总线速率和默认档位集中在 `app/BusRate.h` 配置。
- 需要调速时，优先修改该配置文件，无需在各驱动分散改动。

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

## 📮 项目状态与联系

项目持续维护中。

如果你在使用过程中遇到问题，欢迎联系：

- QQ 邮箱：634591772@qq.com

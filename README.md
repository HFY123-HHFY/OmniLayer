# OmniLayer (FreeRTOS Branch)

这是 OmniLayer 的 FreeRTOS 分支说明文档，重点面向 RTOS 架构、任务编排和多 MCU 切换流程。

## 项目定位

OmniLayer 的目标是让应用逻辑与芯片底层解耦，在同一套工程中支持多 MCU，并保持一致的构建/烧录体验。

- 面向 MCU：STM32F103、STM32F407
- 核心中间件：FreeRTOS Kernel (LTS)
- 构建体系：CMake + Ninja + GCC ARM
- 烧录体系：OpenOCD

## FreeRTOS 分支重点

该分支不是裸机主循环改名，而是明确采用 RTOS 分层开发：

- 中断层只做最小动作（清标志、搬运、通知）
- 周期任务统一承接定时节拍并做软件分频
- 控制台任务负责串口命令交互
- 遥测任务负责低优先级调试输出，避免串口打印干扰周期业务

## 当前目录结构（RTOS 分支）

```text
OmniLayer/
├─ .vscode/                   # VS Code 任务配置
├─ API/                       # MCU 外设统一 API 抽象层
├─ app/                       # 应用层与 RTOS 任务编排
│  ├─ main.c
│  ├─ Control_Rtos/           # RTOS 任务入口与任务组织
│  ├─ Control_Task/           # 中断入口（TIM/USART/EXTI）
│  ├─ Control/                # 控制算法入口（PID等）
│  ├─ My_Usart/               # 串口 RTOS 封装
│  ├─ My_I2c/
│  ├─ My_SPI/
│  ├─ PID/
│  └─ Filter/
├─ BSP/                       # 板级设备封装（LED/KEY/OLED/MPU等）
├─ Core/                      # F103/F407 底层实现
├─ Drivers/                   # 启动文件/CMSIS/厂商头文件
├─ Enroll/                    # 资源注册与板级映射
├─ Middlewares/               # FreeRTOS 与其他中间件
├─ OpenOCD/                   # F103/F407 烧录脚本
├─ SYSTEM/                    # 系统级基础封装
├─ docs/                      # 分支文档（核心/串口/定时器）
├─ CMakeLists.txt
├─ CMakePresets.json
└─ gcc-arm-none-eabi.cmake
```

## 分层职责（RTOS 视角）

| 层级 | 目录 | 职责 |
| --- | --- | --- |
| 任务编排层 | app/Control_Rtos | 创建任务、组织周期逻辑、控制台与遥测策略 |
| 中断桥接层 | app/Control_Task | TIM/USART/EXTI 中断入口，通知任务 |
| 业务控制层 | app/Control | 控制算法（PID等） |
| 外设抽象层 | API | 跨 MCU 统一外设接口 |
| 板级设备层 | BSP | LED/KEY/传感器封装 |
| 注册映射层 | Enroll | 板级资源到 MCU 外设的绑定 |
| 底层实现层 | Core | F103/F407 专属寄存器与驱动实现 |
| 中间件层 | Middlewares | FreeRTOS 内核与可选组件 |

## 构建与烧录

### 默认路径（无需手工改命令）

- F7：执行 Build Debug
- F8：执行 Flash Debug

默认 Debug 预设使用 MCU_TARGET=AUTO，会按工程配置自动解析目标。

### 选择 MCU 路径（弹窗选择）

已配置“弹出式选择 MCU”的任务输入：

- Build Select MCU：弹窗选择 Debug-F103 / Debug-F407 后构建
- Flash Select MCU：弹窗选择 Debug-F103 / Debug-F407 后烧录

用户级快捷键已绑定为：

- Ctrl+Shift+F1：Build Select MCU
- Ctrl+Shift+F2：Flash Select MCU

### 预设说明

CMakePresets 中当前可用：

- Debug（AUTO）
- Debug-F103
- Debug-F407

## RTOS 任务说明（当前主线）

位于 app/Control_Rtos：

- TaskSystemInit：一次性初始化（注册资源、启动驱动、创建常驻任务后自删）
- TaskPeriodicService：消费 1ms 节拍，执行软件分频（2ms 控制入口、1s 时间戳等）
- TaskConsoleService：串口命令输入与交互（例如 rate 命令）
- TaskTelemetryService：低优先级调试打印（由周期任务通知触发）

## 文档索引

- docs/FreeRTOS_core.md：FreeRTOS 内核知识与基础概念
- docs/FreeRTOS_usart.md：串口 RTOS 封装与收发链路说明
- docs/FreeRTOS_timer.md：RTOS 接管定时器的实现思路与实践说明

## 维护建议

- 新增功能优先先定“任务职责边界”，再写代码。
- 不在 ISR 中做复杂业务逻辑，把重活下沉到任务。
- 打印与调试输出尽量低优先级解耦，避免影响周期任务稳定性。
- 切 MCU 时优先使用 Select MCU 任务，减少目标不一致导致的误判。

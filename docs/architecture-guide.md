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
│  app/          应用层                       │  业务逻辑、控制算法、任务调度
│  - main.c      程序入口                     │
│  - Control/    控制逻辑                     │
│  - Control_Task/ 任务调度+中断回调          │
│  - PID/        PID 控制器                   │
│  - Filter/     滤波器                       │
│  - My_I2c/     软件 I2C 驱动               │
│  - My_SPI/     软件 SPI 驱动               │
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
│  每个 API 函数通过 #if ENROLL_MCU_TARGET    │
│  分发到对应 Core 实现                        │
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
│  SYSTEM/       系统层                       │  系统初始化、时钟、延时
│  sys.c / sys.h / Delay.h                   │
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

### 4.5 软件总线 (bit-bang I2C/SPI)

项目目前使用软件模拟的 I2C 和 SPI（非硬件外设），原因可能是：
- 灵活性高，不受硬件 I2C/SPI 实例限制
- 跨平台一致性好（软件模拟行为相同）
- 引脚映射自由

总线速率集中配置在 [app/BusRate.h](app/BusRate.h)，每个 MCU 各自一份。

---

## 5. 当前 API 层支持的外设接口

| API 头文件 | 功能 | G3507 支持 | F103 支持 | F407 支持 |
|-----------|------|:---:|:---:|:---:|
| [gpio.h](API/inc/gpio.h) | GPIO 输入/输出 | ✅ | ✅ | ✅ |
| [usart.h](API/inc/usart.h) | 串口通信 | ✅ | ✅ | ✅ |
| [pwm.h](API/inc/pwm.h) | PWM 输出 | ✅ | ✅ | ✅ |
| [tim.h](API/inc/tim.h) | 定时器中断 | ✅ | ✅ | ✅ |
| [adc.h](API/inc/adc.h) | ADC 采集 | ✅ | ✅ | ✅ |
| [exti.h](API/inc/exti.h) | 外部中断 | ✅ | ❌ (未实现) | ❌ (未实现) |
| [Encoder.h](API/inc/Encoder.h) | 编码器接口 | ✅ (已规划) | ✅ (已规划) | ✅ (已规划) |

**说明：** EXTI 目前仅在 G3507 Core 有实现，从 git commit 记录看最近才解决 G3507 高 IO 脚外部中断的 BUG。

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

## 7. 关键文件索引

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

### 应用层核心
| 文件 | 作用 |
|------|------|
| [app/main.c](app/main.c) | 程序入口，完整的初始化流程 + 主循环 |
| [app/BusRate.h](app/BusRate.h) | I2C/SPI 软件总线速率集中配置 |
| [app/Control_Task/](app/Control_Task/) | 控制任务调度 + 中断回调 |
| [app/PID/](app/PID/) | PID 控制器实现 |
| [app/Filter/](app/Filter/) | 滤波器实现 |
| [app/My_I2c/](app/My_I2c/) | 软件 I2C 驱动 (bit-bang) |
| [app/My_SPI/](app/My_SPI/) | 软件 SPI 驱动 (bit-bang) |
| [app/My_Usart/](app/My_Usart/) | 串口打印封装 |

---

## 8. 开发约定与命名规范

### 函数命名
- `API_xxx_*` — API 层对外接口 (如 `API_GPIO_Write`)
- `F103_xxx_*` — F103 Core 层实现
- `F407_xxx_*` — F407 Core 层实现
- `G3507_xxx_*` — G3507 Core 层实现
- `Enroll_xxx_*` — 注册层门面函数
- `MyI2C_*` / `MySPI_*` — 软件总线驱动

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

## 9. 新增 MCU 的接入步骤

参照 README 描述，实际工程中的接入流程：

1. **Drivers 层** — 在 `Drivers/` 下新增 `Drivers_NewMCU/` 目录，放入启动文件 + SDK/HAL
2. **Core 层** — 在 `Core/NewMCU/` 下实现 `NewMCU_gpio/adc/pwm/tim/usart/...` 等底层驱动
3. **Enroll 层** — 在 `Enroll/` 下新增 `NewMCU_hw_config.h`，定义所有板级映射宏
4. **CMakeLists.txt** — 新增 `elseif(RESOLVED_MCU_TARGET STREQUAL "NewMCU")` 分支
5. **CMakePresets.json** — 新增 `Debug-NewMCU` 构建预设
6. **OpenOCD** — 新增对应的 `.cfg` 下载配置
7. **app/BusRate.h** — 新增该 MCU 的总线速率配置

---

## 10. 当前工程状态

### 最近工作 (基于 git log)
- TI MSPM0G3507 的串口 0/1/2 已调试通，串口 3 尚有乱码问题
- G3507 外部中断 (EXTI) 高位 IO 触发的 BUG 已修复
- 编码器 (Encoder) API 接口已规划，在 G3507_hw_config.h 中有预留头文件引用
- TB6612 电机驱动已三平台通用化
- KEY 驱动已修复多路返回值问题，优化消抖逻辑

### 注意事项
- FreeRTOS-LTS、USB 协议库、TI 官方 SDK 源文件不再同步上传至 GitHub
- 工程保留 MDK_ARM 目录用于 Keil IDE 兼容
- G3507 构建通过 TI DriverLib 实现，区别于 STM32 的标准外设库

---

## 11. 快速上手检查清单

为新对话恢复认知时，按以下顺序阅读关键文件：

1. ✅ 本文档 (docs/architecture-guide.md) — 架构全貌
2. [README.md](README.md) — 项目简介与构建命令
3. [CMakeLists.txt](CMakeLists.txt) — 理解源文件组织与平台分支
4. [Enroll/Enroll.h](Enroll/Enroll.h) — 注册层接口全览
5. [Enroll/G3507_hw_config.h](Enroll/G3507_hw_config.h) — 当前默认 MCU 的板级映射
6. [app/main.c](app/main.c) — 典型的初始化流程
7. 任一 `API/src/*.c` — 理解 API→Core 分发模式
8. 任一 `Core/*/src/*.c` — 理解 Core 层实现风格

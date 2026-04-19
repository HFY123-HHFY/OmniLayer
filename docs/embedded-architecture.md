#  仓库架构说明（CMake + GCC + OpenOCD）

当前仓库已经支持 STM32F103 和 STM32F407 两个目标，后续如果加入新芯片，也按同样规则扩展。

## 1. 分层职责

- `app/`：应用层，只关心业务逻辑和调度。
- `BSP/`：板级支持层，把板子上的 LED、按键、传感器等做成统一接口。
- `Enroll/`：注册层，把 BSP 的硬件资源映射到当前 MCU 上。
- `Core/`：芯片相关驱动层，放 GPIO/USART/TIM/ADC 这类 MCU 直连代码。
- `SYSTEM/`：系统初始化层，放时钟、启动后系统配置等代码。
- `Drivers/`：启动文件、标准外设库、HAL/CMSIS 等 SDK 级资源。
- `oepnocd/`：按 MCU 分类管理 OpenOCD 配置。
- `CMakeLists.txt` / `CMakePresets.json`：统一构建入口与目标选择。

## 2. 工程管理

当前统一使用：

- `build/Debug`
- `build/Release`

芯片目标由 `Enroll/Enroll.h` 里的 `ENROLL_MCU_TARGET` 默认值决定。

## 3. 目标选择方式

根目录 `CMakeLists.txt` 默认会自动读取 `Enroll/Enroll.h` 中的 `ENROLL_MCU_TARGET`，自动选择 F103 或 F407 的源码、链接脚本、下载配置。

## 4. 当前快捷键

- F7：编译（先配置再构建）。
- F8：烧录（先编译再烧录）。

## 5. 新增一个 MCU 时怎么做

1. 在 `Core/`、`SYSTEM/`、`Drivers/Start/` 下补齐该芯片需要的最小启动/系统文件。
2. 在根 `CMakeLists.txt` 里新增一个 MCU 分支。
3. 在 `oepnocd/` 下新增对应下载配置（例如 `xxx_oepnocd.cfg`）。
4. 默认情况下不需要增加新的快捷键，继续沿用 F7/F8。
5. 如果板级资源不同，在 `Enroll/xxx_hw_config.h` 中新增映射。

## 6. 维护原则

- 业务代码尽量不直接碰寄存器。
- 目标相关代码尽量集中在 `Core/`、`SYSTEM/`、`Drivers/Start/`。
- 每个 MCU 保持自己的链接脚本和 OpenOCD 配置。
- 共享的 BSP 接口尽量保持不变，只替换底层注册和驱动实现。

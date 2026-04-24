# 仓库架构说明（CMake + GCC + OpenOCD）

当前仓库已经支持 STM32F103 和 STM32F407 两个目标，后续如果加入新芯片，也按同样规则扩展。

## 1. 分层职责

- `app/`：应用层，只关心业务逻辑和调度。
- `BSP/`：板级支持层，把板子上的 LED、按键、传感器等做成统一接口。
- `Enroll/`：注册层，把 BSP 的硬件资源映射到当前 MCU 上。
- `Core/`：芯片相关驱动层，放 GPIO/USART/TIM/ADC 这类 MCU 直连代码。
- `SYSTEM/`：系统初始化层，放时钟、启动后系统配置等代码。
- `Drivers/`：启动文件、标准外设库、HAL/CMSIS 等 SDK 级资源。
- `OpenOCD/`：按 MCU 分类管理下载配置。
- `CMakeLists.txt` / `CMakePresets.json`：统一构建入口与目标选择。

## 2. 工程管理

当前统一使用：

- `build/Debug`
- `build/Release`

芯片目标由 `Enroll/Enroll.h` 里的 `ENROLL_MCU_TARGET` 默认值决定。

## 3. 目标选择方式

根目录 `CMakeLists.txt` 默认会自动读取 `Enroll/Enroll.h` 中的 `ENROLL_MCU_TARGET`，自动选择 F103 或 F407 的源码、链接脚本和 OpenOCD 配置。

## 4. 当前快捷键

- F7：编译（先配置再构建）。
- F8：烧录（先编译再烧录）。

## 5. 新增一个 MCU 时怎么做

1. 在 `Core/`、`SYSTEM/`、`Drivers/Start/` 下补齐该芯片需要的最小启动/系统文件。
2. 在根 `CMakeLists.txt` 里新增一个 MCU 分支。
3. 在 `OpenOCD/` 下新增对应下载配置（建议命名为 `Fxxx_OpenOCD.cfg`）。
4. 默认情况下不需要增加新的快捷键，继续沿用 F7/F8。
5. 如果板级资源不同，在 `Enroll/xxx_hw_config.h` 中新增映射。

## 6. 维护原则

- 业务代码尽量不直接碰寄存器。
- 目标相关代码尽量集中在 `Core/`、`SYSTEM/`、`Drivers/Start/`。
- 每个 MCU 保持自己的链接脚本和 OpenOCD 配置。
- 共享的 BSP 接口尽量保持不变，只替换底层注册和驱动实现。

## 7. 当前 OpenOCD 配置文件

- `OpenOCD/F103_OpenOCD.cfg`
- `OpenOCD/F407_OpenOCD.cfg`

## 8. 软件 I2C 多总线架构（应用层）

### 8.1 资源注册模型

- 板级 `HW_I2C_MAP(X)` 由 `Enroll` 展开并注册到 `MyI2C_Config_t` 表。
- `MyI2C_Config_t` 包含 `id/port/sclPin/sdaPin`，支持同一固件内多路软件 I2C。
- `MyI2C_Register()` 只做配置表登记，不直接执行外设事务。

### 8.2 运行时选择模型

- `MyI2C_SelectBus(busId)`：切换当前活动总线。
- `MyI2C_SetSpeed(speed)`：切换当前时序档位（100k/400k）。
- 驱动应在每次事务前显式设置总线与速率，避免跨模块状态污染。

### 8.3 当前板级约定

- F103：默认单路 `My_I2C1`（PB8/PB9）。
- F407：
	- `My_I2C1` -> PB8/PB9（MPU6050/QMC5883P/BMP280）
	- `My_I2C2` -> PD5/PD6（OLED）

### 8.4 调试与验线

- `App_I2C_ScanOnce()` 会遍历已注册总线，逐路扫描 7-bit 地址空间并打印总线号。
- 典型输出：`[I2C][My_I2C2] found: 0x3C`。

### 8.5 枚举上界说明

- `My_I2C_MAX` 是 `MyI2C_BusId_t` 的上界/哨兵值，用于边界判断，不代表真实总线实例。

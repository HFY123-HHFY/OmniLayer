--- 构建嵌入式软件开发架构 ---

app：应用层，控制任务与业务逻辑（如 main.c、Control_Task 等）

API：MCU 片内外设统一接口层（如 gpio/tim/usart 抽象接口）

BSP：板级支持层，封装板载外设（LED/KEY/OLED/MPU6050/QMC5883P 等）

Enroll：硬件资源注册层，把 BSP 映射到当前 MCU 的具体引脚与外设实例

Core：MCU 核心驱动实现层，放各芯片族的 GPIO/TIM/USART 等底层代码

MDK_ARM：保留 Keil5 工程与调试配置，兼容不同开发者开发习惯

Middlewares：中间件层（如 FreeRTOS、USB 等）

SYSTEM：系统层，时钟、中断分发、系统级初始化相关代码

Drivers：芯片启动文件、寄存器定义、标准库/HAL/CMSIS 等 SDK 资源

---

入口：

1) 根目录 `CMakeLists.txt`（统一构建入口，默认按 `Enroll/Enroll.h` 中 `ENROLL_MCU_TARGET` 自动选择 F103/F407）。
2) 根目录 `CMakePresets.json`（Debug/Release 预设）。
3) `OpenOCD/F103_OpenOCD.cfg` 与 `OpenOCD/F407_OpenOCD.cfg`（F103/F407 下载配置）。
4) 详细架构说明见 `docs/architecture.md`。

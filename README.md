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

---

应用层软件 I2C 使用说明（多总线）

1) 注册与初始化顺序
- 在 `main` 初始化阶段先调用 `Enroll_I2C_Register()`，再调用 `MyI2C_Init()`。
- `Enroll` 会把板级 `HW_I2C_MAP(X)` 展开为 `MyI2C_Config_t` 配置表并注册。

2) 板级映射写法
- 单路示例：`X(My_I2C1, GPIOB, GPIO_Pin_8, GPIO_Pin_9)`
- 多路示例：
	- `X(My_I2C1, GPIOB, GPIO_Pin_8, GPIO_Pin_9)`
	- `X(My_I2C2, GPIOD, GPIO_Pin_5, GPIO_Pin_6)`

3) 驱动层访问约定
- 每个外设驱动在发起事务前都应明确选择总线和速率：
	- `MyI2C_SelectBus(...)`
	- `MyI2C_SetSpeed(...)`
- 推荐做法：在驱动内部封装 `xxx_SelectI2CSpeed()`，统一设置总线与速率。

4) 当前项目约定（F407）
- `My_I2C1`：MPU6050 / QMC5883P / BMP280（PB8/PB9）
- `My_I2C2`：OLED（PD5/PD6）

5) 扫描与联调
- 调用 `App_I2C_ScanOnce()` 会遍历所有已注册总线，并按总线号打印在线地址。
- 串口输出示例：`[I2C][My_I2C1] found: 0x68`

6) `My_I2C_MAX` 说明
- `My_I2C_MAX` 是总线枚举上界（哨兵值），用于边界判断和遍历上限，不代表可用硬件总线。

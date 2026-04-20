----构建嵌入式软件开发架构---

app 应用层 控制任务 业务代码 main.c pid Control_Task...（软件）

API MCU片内 外设注册层 如gpio.h...MCU资源抽象

BSP: 板级支持包 MCU和板载外设通信 mpu6050 LED KEY ...(硬件抽象层)

Enroll 注册层 把BSP层的硬件抽象在对应的MCU上完成注册并给出接口在app层调用,链接对应MCU和BSP层

Core: MCU驱动程序 MCU上的外设 gpio.c...

MDK_ARM: 保留keil5开发习惯

Middlewares: 中间件 FreeRTOS,USB协议...

SYSTEM 系统配置层

Drivers: SDK程序 CPU驱动库 启动程序 寄存器 标准库 HAL库...(兼容不同开发者习惯)

---

入口：

1) 根目录 `CMakeLists.txt`（统一构建入口，默认按 `Enroll/Enroll.h` 里的 `ENROLL_MCU_TARGET` 选择 F103/F407）
2) 根目录 `CMakePresets.json`（Debug/Release 预设）
3) `oepnocd/f103_openocd.cfg` 与 `oepnocd/f407_oepnocd.cfg`（F103/F407 下载配置）
4) 详细说明见 `docs/embedded-architecture.md`

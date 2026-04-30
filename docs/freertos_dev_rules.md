# FreeRTOS 开发分层规定（OmniLayer）

本文用于统一 FreeRTOS 分支开发方式，避免“代码放哪都行”导致的混乱。

## 1. 为什么需要 Control_Task_Rtos.c/.h

- `app/main.c` 在 RTOS 模式下应当很薄，只负责：
- 最小硬件初始化。
- 创建任务并启动调度器。
- 这时真正的业务循环不再写在 `main`，而是写在任务函数里。

`Control_Task_Rtos.c` 的职责就是“任务层入口与任务编排”：
- 创建任务。
- 设置任务优先级。
- 管理阶段性初始化任务。

`Control_Task_Rtos.h` 只暴露一个入口：`ControlTask_RtosCreate()`，防止上层耦合到具体任务细节。

## 2. 分层职责（RTOS 模式）

- `app/`：任务、状态机、控制算法、业务流程。
- `BSP/`：LED/KEY/传感器等设备抽象。
- `Enroll/`：板级资源注册（引脚/外设映射）。
- `API/`：统一外设接口。
- `Core/`：芯片寄存器级驱动实现。
- `SYSTEM/`：系统级中断注册与系统服务。

## 3. main 为什么只有一个 while(1)

因为调度器接管后，CPU 执行流在各任务之间切换：
- `vTaskStartScheduler()` 之后，任务循环才是“主循环”。
- `main` 末尾的 `while(1)` 只是兜底，不是业务循环。

## 4. 任务参数怎么理解

以 `Task1/Task2` 为例：

- `TASK_LED_STACK_WORDS`
- 含义：该任务栈大小，单位是 word（32 位 MCU 上 1 word = 4 bytes）。
- 例如 `128` 约等于 512 bytes 栈空间。

- `TASK_LED1_PRIORITY` / `TASK_LED2_PRIORITY`
- 含义：任务优先级。
- `tskIDLE_PRIORITY` 是最低优先级基准。
- `+2` 比 `+1` 更高，表示在同等条件下 LED1 任务更先运行。

## 5. 初始化迁移规则（从裸机过渡）

建议分阶段把旧 `main` 初始化搬到 RTOS：

- 第一阶段：保留 `main` 最小初始化，新增 `TaskInit` 做一次性初始化后 `vTaskDelete(NULL)`。
- 第二阶段：把传感器/OLED/通信初始化拆到各自模块任务。
- 第三阶段：删掉裸机轮询逻辑，改成任务调度 + 事件触发。

## 6. 编码规范（本分支）

- 任务函数命名建议：`TaskXxx`。
- 任务创建统一放在 `ControlTask_RtosCreate()`。
- 禁止在任务中使用裸机 `Delay_ms/us`，统一使用 `vTaskDelay` 或事件等待。
- 新增模块先补注释：说明“放在这层的理由”。

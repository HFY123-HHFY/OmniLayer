# FreeRTOS 中断归属与优先级规范

## 1. 三个内核异常归属

以下异常由 FreeRTOS port 独占：

- `SVC_Handler`
- `PendSV_Handler`
- `SysTick_Handler`

它们通过 `FreeRTOSConfig.h` 宏映射到 port 实现，业务代码不要重写。

## 2. 业务中断与 FreeRTOS API 边界

- 如果中断里不调用 FreeRTOS API：可按业务需要设置优先级。
- 如果中断里要调用 `...FromISR` API：
- 中断优先级数值必须大于等于 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`。
- 当前配置为 `5`，即仅优先级 `5~15` 的中断可安全调用 `FromISR` API。

## 3. 当前配置（F103/F407）

- `configPRIO_BITS = 4`
- `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`
- `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`

对应语义：
- 优先级 `0~4`：高优先级中断，不能调 FreeRTOS API。
- 优先级 `5~15`：可调 `FromISR` API。

## 4. 推荐约定

- 实时性极高且不依赖 RTOS 的中断：`0~4`。
- 需要和任务同步（队列/信号量通知）的中断：`5~15`。
- 严禁在 ISR 中调用阻塞式 API（如 `xQueueSend` 非 FromISR 版本）。

## 5. 迁移检查清单

- 检查是否错误实现了 `SVC_Handler/PendSV_Handler/SysTick_Handler`。
- 检查所有 ISR 里是否混用了非 `FromISR` API。
- 检查使用 `FromISR` API 的中断优先级是否在 `5~15`。

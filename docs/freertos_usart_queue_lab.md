# FreeRTOS 串口封装使用说明（My_Usart 版）

## 1. 重构后分层职责

1. `app/My_Usart/My_Usart.c`
- 串口底层发送（原有 `usart_printf` / `printf` 重定向）
- RTOS 串口封装（RX/TX 队列、信号量、互斥锁、后台任务）

2. `app/Control_Task/Control_Task_Rtos.c`
- 只负责初始化和业务任务
- 业务任务通过 My_Usart 提供的 RTOS 接口收发数据

3. `app/Control_Task/Control_Task.c`
- `USART1_IRQHandler` 的 RXNE 分支只调用 `MyUsart_RtosRxIrqHandler(USART1)`

## 2. RTOS 串口封装用了哪些对象

1. Queue
- RX 字节队列：中断字节 -> RX 后台任务
- TX 消息队列：业务任务消息 -> TX 后台任务
- RX 行队列：RX 后台任务组包后 -> 业务命令任务

2. Binary Semaphore
- RX 中断通知 RX 后台任务“有新字节到达”

3. Mutex
- TX 后台任务发送时互斥保护，防止并发任务输出冲突

## 3. 发送怎么用

### A. 兼容旧接口（直接发）

1. `printf("value=%d\\r\\n", v);`
2. `usart_printf(USART1, "rpm=%d\\r\\n", rpm);`

说明：这两种仍可用，走的是原有 My_Usart 发送路径。

### B. 推荐 RTOS 接口（任务里发）

1. `MyUsart_RtosSendText("hello\\r\\n");`
2. `MyUsart_RtosPrintf("cnt=%lu\\r\\n", cnt);`

说明：这两种会进入 TX 队列，由 TX 后台任务统一发送。

## 4. 接收怎么用

### A. 中断里怎么写

`USART1_IRQHandler` 的 RXNE 分支：

```c
if ((USART1->SR & USART_SR_RXNE) != 0U)
{
	MyUsart_RtosRxIrqHandler(USART1);
}
```

说明：中断里只做字节搬运和通知，不做命令解析。

### B. 任务里怎么收

```c
char line[64];
if (MyUsart_RtosRecvLine(line, sizeof(line), 1000U) != 0U)
{
	/* line 是一整行命令，不含 CR/LF */
}
```

## 5. 初始化顺序（必须）

1. `Enroll_USART_Register();`
2. `API_USART_Init(API_USART1, 115200U);`
3. `MyUsart_RtosStart(USART1);`

之后业务任务就可以调用 `MyUsart_RtosSendText / MyUsart_RtosPrintf / MyUsart_RtosRecvLine`。

## 6. 你现在工程里的最小示例

1. 心跳任务：每 1s 调用 `MyUsart_RtosPrintf` 发送心跳
2. 命令任务：调用 `MyUsart_RtosRecvLine` 读取 `help/ping/rate` 并回复

## 7. 常见注意点

1. 不要在 ISR 里调用会阻塞的 API
2. 命令解析放任务里，不要放中断里
3. 多任务发串口时优先走 RTOS 封装发送接口

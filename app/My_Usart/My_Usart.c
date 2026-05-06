#include "My_Usart.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include <sys/stat.h>

/*
 * 发送环形队列结构：
 * - head: 生产者写入位置（主循环或任务上下文）
 * - tail: 消费者取出位置（TXE 中断上下文）
 */

typedef struct
{
	USART_TypeDef *instance;
	volatile uint16_t head;
	volatile uint16_t tail;
	uint8_t buf[USART_TX_BUF_SIZE];
} USART_TxAsyncQueue;

/* 每个 USART 实例对应一套异步发送队列。 */
static USART_TxAsyncQueue g_usart_tx_q1 = {USART1, 0U, 0U, {0}};
static USART_TxAsyncQueue g_usart_tx_q2 = {USART2, 0U, 0U, {0}};
static USART_TxAsyncQueue g_usart_tx_q3 = {USART3, 0U, 0U, {0}};

/* 全局接收解析状态。 */
USART_DataType USART_DataTypeStruct;

/* ======================== RTOS 串口封装区 ======================== */
#define MYUSART_RTOS_RX_BYTE_QUEUE_LEN   (64U)
#define MYUSART_RTOS_TX_MSG_QUEUE_LEN    (16U)
#define MYUSART_RTOS_RX_LINE_QUEUE_LEN   (8U)
#define MYUSART_RTOS_TX_MSG_MAX_LEN      (96U)
#define MYUSART_RTOS_RX_LINE_MAX_LEN     (64U)

#define MYUSART_RTOS_TX_TASK_STACK_WORDS (256U)
#define MYUSART_RTOS_RX_TASK_STACK_WORDS (256U)
#define MYUSART_RTOS_TX_TASK_PRIORITY    (tskIDLE_PRIORITY + 2U)
#define MYUSART_RTOS_RX_TASK_PRIORITY    (tskIDLE_PRIORITY + 2U)

typedef struct
{
	char text[MYUSART_RTOS_TX_MSG_MAX_LEN];
} MyUsartRtosTxMsg_t;

typedef struct
{
	char line[MYUSART_RTOS_RX_LINE_MAX_LEN];
} MyUsartRtosRxLine_t;

static QueueHandle_t s_rtosRxByteQueue;  /* ISR -> RX任务 */
static QueueHandle_t s_rtosTxMsgQueue;   /* App任务 -> TX任务 */
static QueueHandle_t s_rtosRxLineQueue;  /* RX任务 -> App任务 */
static SemaphoreHandle_t s_rtosRxSem;    /* ISR 到 RX任务通知 */
static SemaphoreHandle_t s_rtosTxMutex;  /* 串口发送互斥保护 */
static USART_TypeDef *s_rtosUsart;       /* RTOS封装绑定的串口实例 */
static uint8_t s_rtosStarted;

/* 前置声明：RTOS 串口后台任务。 */
static void MyUsart_RtosTxTask(void *pvParameters);
static void MyUsart_RtosRxTask(void *pvParameters);

/* 根据 USART 实例返回对应发送队列。 */
static USART_TxAsyncQueue *usart_get_tx_queue(USART_TypeDef *USARTx)
{
	if (USARTx == USART1)
	{
		return &g_usart_tx_q1;
	}
	if (USARTx == USART2)
	{
		return &g_usart_tx_q2;
	}
	if (USARTx == USART3)
	{
		return &g_usart_tx_q3;
	}

	return 0;
}

/*
 * 把 USARTx 寄存器实例转换为 API 层 ID。
 * 这样可以复用 API_USART_WriteByte 完成阻塞兜底发送。
 */
static uint8_t usart_instance_to_id(USART_TypeDef *USARTx, API_USART_Id_t *id)
{
	if (id == 0)
	{
		return 0U;
	}

	if (USARTx == USART1)
	{
		*id = API_USART1;
		return 1U;
	}
	if (USARTx == USART2)
	{
		*id = API_USART2;
		return 1U;
	}
	if (USARTx == USART3)
	{
		*id = API_USART3;
		return 1U;
	}
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	if (USARTx == USART4)
	{
		*id = API_USART4;
		return 1U;
	}
#endif

	return 0U;
}

/* 进入临界区：返回进入前 PRIMASK 状态。 */
static uint32_t usart_enter_critical(void)
{
	uint32_t primask;

	__asm volatile("MRS %0, PRIMASK" : "=r"(primask));
	__asm volatile("cpsid i" : : : "memory");
	return primask;
}

/* 退出临界区：仅在进入前中断开放时恢复中断。 */
static void usart_exit_critical(uint32_t primask)
{
	if ((primask & 0x1U) == 0U)
	{
		__asm volatile("cpsie i" : : : "memory");
	}
}

/*
 * 发送 1 字节：
 * 1) 先尝试异步入队；
 * 2) 入队失败（队列满/实例不支持）时，退化为阻塞发送兜底。
 */
void usart_send_byte(USART_TypeDef *USARTx, uint8_t Byte)
{
	API_USART_Id_t id;

	if (usart_send_byte_async(USARTx, Byte) != 0U)
	{
		return;
	}

	if (usart_instance_to_id(USARTx, &id) == 0U)
	{
		return;
	}

	API_USART_WriteByte(id, Byte);
}

/*
 * 异步发送 1 字节：
 * - 成功：写入队列并打开 TXEIE，中断后续搬运发送；
 * - 失败：返回 0（队列满或实例不支持）。
 */
uint8_t usart_send_byte_async(USART_TypeDef *USARTx, uint8_t Byte)
{
	uint32_t primask;
	uint16_t next_head;
	USART_TxAsyncQueue *q;

	q = usart_get_tx_queue(USARTx);
	if (q == 0)
	{
		return 0U;
	}

	primask = usart_enter_critical();

	next_head = (uint16_t)((q->head + 1U) % USART_TX_BUF_SIZE);
	if (next_head == q->tail)
	{
		usart_exit_critical(primask);
		return 0U;
	}

	q->buf[q->head] = Byte;
	q->head = next_head;
	q->instance->CR1 |= USART_CR1_TXEIE;

	usart_exit_critical(primask);
	return 1U;
}

/* 发送 C 字符串（逐字节调用 usart_send_byte）。 */
void usart_SendString(USART_TypeDef *USARTx, const char *String)
{
	uint16_t i;

	if (String == 0)
	{
		return;
	}

	for (i = 0U; String[i] != '\0'; i++)
	{
		usart_send_byte(USARTx, (uint8_t)String[i]);
	}
}

/* 数字转十进制字符串后发送。 */
void usart_send_number(USART_TypeDef *USARTx, uint32_t Number)
{
	char String[11];

	(void)snprintf(String, sizeof(String), "%lu", (unsigned long)Number);
	usart_SendString(USARTx, String);
}

/* 简单幂函数，供上层保留兼容调用。 */
uint32_t usart_pow(uint32_t X, uint32_t Y)
{
	uint32_t Result;

	Result = 1U;
	while (Y--)
	{
		Result *= X;
	}

	return Result;
}

/* 连续发送字节数组。 */
void usart_send_array(USART_TypeDef *USARTx, uint8_t *Array, uint16_t Length)
{
	uint16_t i;

	if (Array == 0)
	{
		return;
	}

	for (i = 0U; i < Length; i++)
	{
		usart_send_byte(USARTx, Array[i]);
	}
}

/* printf 字符输出重定向。 */
int fputc(int ch, FILE *f)
{
	(void)f;
	usart_send_byte(PRINTF_USART, (uint8_t)ch);
	return ch;
}

/*
 * newlib-nano 的 printf 通常走 _write，而不是逐字符调用 fputc。
 * 实现 _write 后，printf("...") 才会真正从串口输出。
 */
int _write(int file, char *ptr, int len)
{
	int i;

	(void)file;
	if ((ptr == 0) || (len <= 0))
	{
		return 0;
	}

	for (i = 0; i < len; i++)
	{
		usart_send_byte(PRINTF_USART, (uint8_t)ptr[i]);
	}

	return len;
}

/* newlib-nano 最小 syscalls 桩，避免链接阶段未实现告警。 */
int _close(int file)
{
	(void)file;
	return -1;
}

int _fstat(int file, struct stat *st)
{
	(void)file;
	if (st != 0)
	{
		st->st_mode = S_IFCHR;
	}
	return 0;
}

int _getpid(void)
{
	return 1;
}

int _isatty(int file)
{
	(void)file;
	return 1;
}

int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	return -1;
}

int _lseek(int file, int ptr, int dir)
{
	(void)file;
	(void)ptr;
	(void)dir;
	return 0;
}

int _read(int file, char *ptr, int len)
{
	(void)file;
	(void)ptr;
	(void)len;
	return 0;
}

/* 格式化输出：先格式化到本地缓冲，再统一发送。 */
void usart_printf(USART_TypeDef *USARTx, const char *format, ...)
{
	char String[128];
	int len;
	va_list arg;

	va_start(arg, format);
	len = vsnprintf(String, sizeof(String), format, arg);
	va_end(arg);

	if (len <= 0)
	{
		return;
	}

	usart_SendString(USARTx, String);
}

/*
 * TXE 中断服务分发函数：
 * - 队列有数据：写 DR 并前移 tail；
 * - 队列为空：关闭 TXEIE，避免空中断反复触发。
 */
void usart_tx_irq_handler(USART_TypeDef *USARTx)
{
	USART_TxAsyncQueue *q;

	q = usart_get_tx_queue(USARTx);
	if (q == 0)
	{
		return;
	}

	if (q->tail != q->head)
	{
		q->instance->DR = q->buf[q->tail];
		q->tail = (uint16_t)((q->tail + 1U) % USART_TX_BUF_SIZE);
	}
	else
	{
		q->instance->CR1 &= ~USART_CR1_TXEIE;
	}
}

/*
 * RTOS 发送接口：把文本交给 TX 队列，由 TX 任务统一发送。
 * 设计目的：避免多个业务任务同时直写串口，导致输出交叉。
 */
uint8_t MyUsart_RtosSendText(const char *text)
{
	MyUsartRtosTxMsg_t msg;

	if ((text == 0) || (s_rtosStarted == 0U) || (s_rtosTxMsgQueue == 0))
	{
		return 0U;
	}

	(void)memset(&msg, 0, sizeof(msg));
	(void)snprintf(msg.text, sizeof(msg.text), "%s", text);

	if (xQueueSend(s_rtosTxMsgQueue, &msg, pdMS_TO_TICKS(20U)) != pdPASS)
	{
		return 0U;
	}

	return 1U;
}

/* RTOS 版 printf：格式化后进入 TX 队列。 */
uint8_t MyUsart_RtosPrintf(const char *format, ...)
{
	char line[MYUSART_RTOS_TX_MSG_MAX_LEN];
	va_list arg;
	int len;

	if (format == 0)
	{
		return 0U;
	}

	va_start(arg, format);
	len = vsnprintf(line, sizeof(line), format, arg);
	va_end(arg);

	if (len <= 0)
	{
		return 0U;
	}

	return MyUsart_RtosSendText(line);
}

/*
 * App 任务从 RX 行队列取一整行命令。
 * timeoutMs 为等待超时时间（毫秒）。
 */
uint8_t MyUsart_RtosRecvLine(char *lineBuf, uint16_t bufSize, uint32_t timeoutMs)
{
	MyUsartRtosRxLine_t lineMsg;

	if ((lineBuf == 0) || (bufSize == 0U) || (s_rtosStarted == 0U) || (s_rtosRxLineQueue == 0))
	{
		return 0U;
	}

	if (xQueueReceive(s_rtosRxLineQueue, &lineMsg, pdMS_TO_TICKS(timeoutMs)) != pdPASS)
	{
		return 0U;
	}

	(void)snprintf(lineBuf, bufSize, "%s", lineMsg.line);
	return 1U;
}

/*
 * USART RXNE 中断桥接：
 * 1) 在 ISR 里读取 DR 清除 RXNE；
 * 2) 字节入队；
 * 3) 给 RX 任务信号量。
 */
void MyUsart_RtosRxIrqHandler(USART_TypeDef *USARTx)
{
	BaseType_t xHigherPriorityTaskWoken;
	uint8_t data;

	if (USARTx == 0)
	{
		return;
	}

	data = (uint8_t)USARTx->DR;

	if ((s_rtosStarted == 0U) || (USARTx != s_rtosUsart) || (s_rtosRxByteQueue == 0) || (s_rtosRxSem == 0))
	{
		return;
	}

	xHigherPriorityTaskWoken = pdFALSE;
	(void)xQueueSendFromISR(s_rtosRxByteQueue, &data, &xHigherPriorityTaskWoken);
	(void)xSemaphoreGiveFromISR(s_rtosRxSem, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*
 * 启动 RTOS 串口封装：
 * - 创建队列、信号量、互斥锁；
 * - 创建 TX/RX 后台任务。
 */
uint8_t MyUsart_RtosStart(USART_TypeDef *USARTx)
{
	if (USARTx == 0)
	{
		return 0U;
	}

	if (s_rtosStarted != 0U)
	{
		return 1U;
	}

	s_rtosUsart = USARTx;
	s_rtosRxByteQueue = xQueueCreate(MYUSART_RTOS_RX_BYTE_QUEUE_LEN, sizeof(uint8_t));
	s_rtosTxMsgQueue = xQueueCreate(MYUSART_RTOS_TX_MSG_QUEUE_LEN, sizeof(MyUsartRtosTxMsg_t));
	s_rtosRxLineQueue = xQueueCreate(MYUSART_RTOS_RX_LINE_QUEUE_LEN, sizeof(MyUsartRtosRxLine_t));
	s_rtosRxSem = xSemaphoreCreateBinary();
	s_rtosTxMutex = xSemaphoreCreateMutex();

	if ((s_rtosRxByteQueue == 0) || (s_rtosTxMsgQueue == 0) || (s_rtosRxLineQueue == 0) ||
		(s_rtosRxSem == 0) || (s_rtosTxMutex == 0))
	{
		return 0U;
	}

	if (xTaskCreate(MyUsart_RtosTxTask,
					"uart_tx",
					MYUSART_RTOS_TX_TASK_STACK_WORDS,
					0,
					MYUSART_RTOS_TX_TASK_PRIORITY,
					0) != pdPASS)
	{
		return 0U;
	}

	if (xTaskCreate(MyUsart_RtosRxTask,
					"uart_rx",
					MYUSART_RTOS_RX_TASK_STACK_WORDS,
					0,
					MYUSART_RTOS_RX_TASK_PRIORITY,
					0) != pdPASS)
	{
		return 0U;
	}

	s_rtosStarted = 1U;
	return 1U;
}

/* TX 后台任务：唯一串口发送出口。 */
static void MyUsart_RtosTxTask(void *pvParameters)
{
	MyUsartRtosTxMsg_t msg;

	(void)pvParameters;

	for (;;)
	{
		if (xQueueReceive(s_rtosTxMsgQueue, &msg, portMAX_DELAY) == pdPASS)
		{
			(void)xSemaphoreTake(s_rtosTxMutex, portMAX_DELAY);
			usart_SendString(s_rtosUsart, msg.text);
			(void)xSemaphoreGive(s_rtosTxMutex);
		}
	}
}

/*
 * RX 后台任务：
 * - 接收 ISR 投递的字节流；
 * - 以 \r/\n 为分隔组装成一行；
 * - 把整行放入行队列，供业务任务读取。
 */
static void MyUsart_RtosRxTask(void *pvParameters)
{
	uint8_t rxByte;
	uint8_t lineLen;
	MyUsartRtosRxLine_t lineMsg;

	(void)pvParameters;
	lineLen = 0U;
	(void)memset(&lineMsg, 0, sizeof(lineMsg));

	for (;;)
	{
		if (xSemaphoreTake(s_rtosRxSem, portMAX_DELAY) != pdPASS)
		{
			continue;
		}

		while (xQueueReceive(s_rtosRxByteQueue, &rxByte, 0U) == pdPASS)
		{
			if ((rxByte == '\r') || (rxByte == '\n'))
			{
				if (lineLen == 0U)
				{
					continue;
				}

				lineMsg.line[lineLen] = '\0';
				(void)xQueueSend(s_rtosRxLineQueue, &lineMsg, 0U);
				lineLen = 0U;
				(void)memset(&lineMsg, 0, sizeof(lineMsg));
			}
			else if (lineLen < (uint8_t)(sizeof(lineMsg.line) - 1U))
			{
				lineMsg.line[lineLen] = (char)rxByte;
				lineLen++;
			}
			else
			{
				lineLen = 0U;
				(void)memset(&lineMsg, 0, sizeof(lineMsg));
				(void)MyUsart_RtosSendText("rx line too long, dropped\\r\\n");
			}
		}
	}
}

/*
 * 串口数据包解析：
 * 协议格式：s12,-34,56e
 * 解析完成后：state=2，可通过 USART_Deal 读取 data[]。
 */
void usart_Dispose_Data(USART_TypeDef *USARTx, USART_DataType *USART_DataTypeStruct, uint8_t RxData)
{
	(void)USARTx;

	switch (USART_DataTypeStruct->state)
	{
	case 0:
		if (RxData == 's')
		{
			USART_DataTypeStruct->state = 1U;
			USART_DataTypeStruct->current_index = 0U;
			USART_DataTypeStruct->buffer_len = 0U;
			memset(USART_DataTypeStruct->buffer, 0, sizeof(USART_DataTypeStruct->buffer));
		}
		break;

	case 1:
		if (RxData == 'e')
		{
			if (USART_DataTypeStruct->buffer_len > 0U)
			{
				int16_t value;
				uint8_t i;
				uint8_t is_negative;

				value = 0;
				i = 0U;
				is_negative = 0U;
				if (USART_DataTypeStruct->buffer[0] == '-')
				{
					is_negative = 1U;
					i = 1U;
				}

				for (; i < USART_DataTypeStruct->buffer_len; i++)
				{
					if ((USART_DataTypeStruct->buffer[i] >= '0') && (USART_DataTypeStruct->buffer[i] <= '9'))
					{
						value = (int16_t)(value * 10 + (USART_DataTypeStruct->buffer[i] - '0'));
					}
					else
					{
						USART_DataTypeStruct->state = 0U;
						break;
					}
				}

				if (is_negative != 0U)
				{
					value = (int16_t)(-value);
				}

				if (USART_DataTypeStruct->current_index < Data_len)
				{
					USART_DataTypeStruct->data[USART_DataTypeStruct->current_index] = (uint16_t)value;
					USART_DataTypeStruct->count = (uint8_t)(USART_DataTypeStruct->current_index + 1U);
				}
			}
			USART_DataTypeStruct->state = 2U;
		}
		else if (RxData == ',')
		{
			if (USART_DataTypeStruct->buffer_len > 0U)
			{
				int16_t value;
				uint8_t i;
				uint8_t is_negative;

				value = 0;
				i = 0U;
				is_negative = 0U;
				if (USART_DataTypeStruct->buffer[0] == '-')
				{
					is_negative = 1U;
					i = 1U;
				}

				for (; i < USART_DataTypeStruct->buffer_len; i++)
				{
					if ((USART_DataTypeStruct->buffer[i] >= '0') && (USART_DataTypeStruct->buffer[i] <= '9'))
					{
						value = (int16_t)(value * 10 + (USART_DataTypeStruct->buffer[i] - '0'));
					}
					else
					{
						USART_DataTypeStruct->state = 0U;
						break;
					}
				}

				if (is_negative != 0U)
				{
					value = (int16_t)(-value);
				}

				if (USART_DataTypeStruct->current_index < Data_len)
				{
					USART_DataTypeStruct->data[USART_DataTypeStruct->current_index] = (uint16_t)value;
					USART_DataTypeStruct->current_index++;
				}

				USART_DataTypeStruct->buffer_len = 0U;
				memset(USART_DataTypeStruct->buffer, 0, sizeof(USART_DataTypeStruct->buffer));
			}
		}
		else if (((RxData >= '0') && (RxData <= '9')) || (RxData == '-'))
		{
			if (USART_DataTypeStruct->buffer_len < 15U)
			{
				if ((RxData == '-') && (USART_DataTypeStruct->buffer_len != 0U))
				{
					USART_DataTypeStruct->state = 0U;
				}
				else
				{
					USART_DataTypeStruct->buffer[USART_DataTypeStruct->buffer_len++] = RxData;
				}
			}
			else
			{
				USART_DataTypeStruct->state = 0U;
			}
		}
		else
		{
			USART_DataTypeStruct->state = 0U;
		}
		break;

	case 2:
		if (RxData == 's')
		{
			USART_DataTypeStruct->state = 1U;
			USART_DataTypeStruct->current_index = 0U;
			USART_DataTypeStruct->count = 0U;
			USART_DataTypeStruct->buffer_len = 0U;
			memset(USART_DataTypeStruct->buffer, 0, sizeof(USART_DataTypeStruct->buffer));
		}
		break;

	default:
		USART_DataTypeStruct->state = 0U;
		break;
	}
}

/* 安全读取解析结果。 */
int16_t USART_Deal(USART_DataType *pData, int8_t index)
{
	if ((pData == 0) || (index < 0) || ((uint8_t)index >= pData->count))
	{
		return 0;
	}

	return (int16_t)pData->data[(uint8_t)index];
}

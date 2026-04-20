#ifndef __MY_USART_H
#define __MY_USART_H

/*
 * My_Usart 模块说明：
 * 1) 统一提供“应用层可直接调用”的串口发送/printf/数据包解析接口；
 * 2) 保持与原标准库封装接近的函数名和调用方式；
 * 3) 底层适配当前工程 API 层(usart.h)与 F103/F407 双平台寄存器视图。
 */

#include "Enroll.h"
#include "usart.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* 异步发送环形缓冲区大小（字节）。 */
#ifndef USART_TX_BUF_SIZE
#define USART_TX_BUF_SIZE 512U
#endif

/* printf 默认输出串口（可在编译参数或上层头文件中重定义）。 */
#ifndef PRINTF_USART
#define PRINTF_USART USART1
#endif

/* CR1.TXEIE：发送寄存器空中断使能位。 */
#ifndef USART_CR1_TXEIE
#define USART_CR1_TXEIE (1UL << 7)
#endif

/* 解析数据包后最多保存的数据项个数。 */
#define Data_len 10U

/*
 * USART_TypeDef 统一别名：
 * - F103: 对应 F103_USART_View_t
 * - F407: 对应 F407_USART_View_t
 */
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
typedef F103_USART_View_t USART_TypeDef;
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
typedef F407_USART_View_t USART_TypeDef;
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

/*
 * 串口数据包解析状态结构：
 * 协议示例：s12,-34,56e
 * - 's'：包头
 * - ','：分隔符
 * - 'e'：包尾
 */
typedef struct
{
	uint16_t data[Data_len];
	uint8_t count;
	uint8_t state;
	uint8_t current_index;
	uint8_t buffer[16];
	uint8_t buffer_len;
} USART_DataType;

/* 全局解析状态实例，建议在中断中喂数据，在主循环中读取结果。 */
extern USART_DataType USART_DataTypeStruct;

/*
 * 发送单字节（优先异步，不行则退化阻塞发送）。
 * 参数：USARTx 选择串口实例，Byte 为待发送字节。
 */
void usart_send_byte(USART_TypeDef *USARTx, uint8_t Byte);

/*
 * 发送单字节（纯异步）。
 * 返回：1=入队成功，0=队列满或串口不支持。
 */
uint8_t usart_send_byte_async(USART_TypeDef *USARTx, uint8_t Byte);

/* 发送以 '\0' 结尾的字符串。 */
void usart_SendString(USART_TypeDef *USARTx, const char *String);

/* 发送 32 位无符号整数（十进制字符串形式）。 */
void usart_send_number(USART_TypeDef *USARTx, uint32_t Number);

/* 幂函数：返回 X^Y。 */
uint32_t usart_pow(uint32_t X, uint32_t Y);

/* 连续发送数组中的 Length 个字节。 */
void usart_send_array(USART_TypeDef *USARTx, uint8_t *Array, uint16_t Length);

/* printf 重定向接口：默认发送到 PRINTF_USART。 */
int fputc(int ch, FILE *f);

/*
 * 类 printf 串口输出。
 * 用法：usart_printf(USART1, "rpm=%d\r\n", rpm);
 */
void usart_printf(USART_TypeDef *USARTx, const char *format, ...);

/*
 * TXE 中断处理函数：
 * 需要在对应 USARTx_IRQHandler 的 TXE 分支里调用。
 */
void usart_tx_irq_handler(USART_TypeDef *USARTx);

/*
 * 接收数据包解析函数：
 * 建议在 RXNE 分支读取到 data 后调用。
 */
void usart_Dispose_Data(USART_TypeDef *USARTx, USART_DataType *USART_DataTypeStruct, uint8_t RxData);

/*
 * 获取已解析数据项。
 * 返回：索引有效则返回数据，否则返回 0。
 */
int16_t USART_Deal(USART_DataType *pData, int8_t index);

#endif

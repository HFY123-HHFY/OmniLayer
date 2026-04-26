#ifndef __NRF24L01_H
#define __NRF24L01_H

#include <stdint.h>

#include "My_SPI.h"
#include "BusRate.h"
#include "NRF24L01_Define.h"

/*
 * NRF24L01 驱动说明：
 * 1) 标准 SPI 时序由 My_SPI 提供；
 * 2) 模块独有控制脚 CE 由注册层单独下发；
 * 3) 当前实现采用查询方式收发，不引入 IRQ 中断脚。
 */

/* NRF24L01 使用固定 5 字节地址与 32 字节包长。 */
#define NRF24L01_ADDR_WIDTH      5U
#define NRF24L01_TX_PACKET_WIDTH 32U
#define NRF24L01_RX_PACKET_WIDTH 32U

/* NRF24L01 模块专有控制脚（仅 CE，IRQ 暂不引入）。 */
typedef struct
{
	void *cePort;
	uint16_t cePin;
} NRF24L01_CtrlConfig_t;

/*
 * NRF24L01 绑定的软件 SPI 总线与速率
 * 默认使用 My_SPI2 + 1MHz
 */
#define NRF24L01_SPI_BUS   My_SPI2
#define NRF24L01_SPI_SPEED NRF24L01_SPI_SPEED_PROFILE

/* 发送地址与发送数据包。 */
extern uint8_t NRF24L01_TxAddress[NRF24L01_ADDR_WIDTH];
extern uint8_t NRF24L01_TxPacket[NRF24L01_TX_PACKET_WIDTH];

/* 接收地址与接收数据包。 */
extern uint8_t NRF24L01_RxAddress[NRF24L01_ADDR_WIDTH];
extern uint8_t NRF24L01_RxPacket[NRF24L01_RX_PACKET_WIDTH];

/* 注册 NRF24L01 专有控制脚（CE）。 */
void NRF24L01_RegisterCtrl(const NRF24L01_CtrlConfig_t *configTable, uint8_t count);

/* 指令实现：寄存器与载荷操作。 */
uint8_t NRF24L01_ReadReg(uint8_t regAddress);
void NRF24L01_ReadRegs(uint8_t regAddress, uint8_t *dataArray, uint8_t count);
void NRF24L01_WriteReg(uint8_t regAddress, uint8_t data);
void NRF24L01_WriteRegs(uint8_t regAddress, const uint8_t *dataArray, uint8_t count);
void NRF24L01_ReadRxPayload(uint8_t *dataArray, uint8_t count);
void NRF24L01_WriteTxPayload(const uint8_t *dataArray, uint8_t count);
void NRF24L01_FlushTx(void);
void NRF24L01_FlushRx(void);
uint8_t NRF24L01_ReadStatus(void);

/* 功能接口：模式切换、初始化、收发。 */
void NRF24L01_PowerDown(void);
void NRF24L01_StandbyI(void);
void NRF24L01_Rx(void);
void NRF24L01_Tx(void);
void NRF24L01_Init(void);
uint8_t NRF24L01_Send(void);
uint8_t NRF24L01_Receive(void);
void NRF24L01_UpdateRxAddress(void);

/* 最小回环读写测试：校验寄存器读写链路是否正常。 */
void App_NRF24L01_TestOnce(void);

#endif /* __NRF24L01_H */

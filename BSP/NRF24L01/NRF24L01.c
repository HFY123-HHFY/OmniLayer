#include "NRF24L01.h"

#include "Delay.h"
#include "My_Usart/My_Usart.h"
#include "gpio.h"

/*
 * NRF24L01 模块私有状态：
 * - CE 控制脚由 Enroll 注册层下发；
 * - 地址与收发包缓冲对上层开放，便于应用直接改写。
 */
static const NRF24L01_CtrlConfig_t *s_nrfCtrlTable;
static uint8_t s_nrfCtrlCount;

uint8_t NRF24L01_TxAddress[NRF24L01_ADDR_WIDTH] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U};
uint8_t NRF24L01_TxPacket[NRF24L01_TX_PACKET_WIDTH];

uint8_t NRF24L01_RxAddress[NRF24L01_ADDR_WIDTH] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U};
uint8_t NRF24L01_RxPacket[NRF24L01_RX_PACKET_WIDTH];

static const NRF24L01_CtrlConfig_t *NRF24L01_GetCtrlConfig(void)
{
	if ((s_nrfCtrlTable == 0) || (s_nrfCtrlCount == 0U))
	{
		return 0;
	}

	return &s_nrfCtrlTable[0];
}

/* 注册 NRF24L01 专有控制脚（CE）。 */
void NRF24L01_RegisterCtrl(const NRF24L01_CtrlConfig_t *configTable, uint8_t count)
{
	s_nrfCtrlTable = configTable;
	s_nrfCtrlCount = count;
}

/* 选择 NRF24L01 绑定的软件 SPI 总线。 */
static void NRF24L01_SelectSPIBus(void)
{
	MySPI_SelectBus(NRF24L01_SPI_BUS);
}

/* 选择 NRF24L01 专用 SPI 速率档位（默认 1MHz）*/
static void NRF24L01_SelectSPISpeed(void)
{
	MySPI_SetSpeed(NRF24L01_SPI_SPEED);
}

/*
 * 选择 NRF24L01 SPI 通道参数：
 * 与 OLED 的总线/速率选择模式保持一致，避免跨设备互相污染。
 */
static void NRF24L01_SelectSPI(void)
{
	NRF24L01_SelectSPIBus();
	NRF24L01_SelectSPISpeed();
}

/* 写 CE 电平：CE=1 进入收发态，CE=0 退出收发态。 */
static void NRF24L01_W_CE(uint8_t bitValue)
{
	const NRF24L01_CtrlConfig_t *config;

	config = NRF24L01_GetCtrlConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_Write(config->cePort, config->cePin, bitValue);
}

/* SPI 单字节交换：底层仍由 My_SPI 统一实现。 */
static uint8_t NRF24L01_SPI_SwapByte(uint8_t byteValue)
{
	NRF24L01_SelectSPI();
	return MySPI_SwapByte(byteValue);
}

/*
 * NRF24L01 端口初始化：
 * - SPI 引脚由 MySPI_Init 统一初始化；
 * - CE 由本模块单独初始化并默认拉低。
 */
static void NRF24L01_GPIO_Init(void)
{
	const NRF24L01_CtrlConfig_t *ctrl;

	NRF24L01_SelectSPI();
	MySPI_Init();

	ctrl = NRF24L01_GetCtrlConfig();
	if (ctrl != 0)
	{
		API_GPIO_InitOutput(ctrl->cePort, ctrl->cePin);
		NRF24L01_W_CE(0U);
	}

	MySPI_W_SS(1U);
	MySPI_W_SCK(0U);
	MySPI_W_MOSI(0U);
}

/* 读单寄存器。 */
uint8_t NRF24L01_ReadReg(uint8_t regAddress)
{
	uint8_t data;

	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte((uint8_t)(NRF24L01_R_REGISTER | regAddress));
	data = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
	MySPI_Stop();

	return data;
}

/* 连续读取寄存器。 */
void NRF24L01_ReadRegs(uint8_t regAddress, uint8_t *dataArray, uint8_t count)
{
	uint8_t i;

	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte((uint8_t)(NRF24L01_R_REGISTER | regAddress));
	for (i = 0U; i < count; i++)
	{
		dataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
	}
	MySPI_Stop();
}

/* 写单寄存器。 */
void NRF24L01_WriteReg(uint8_t regAddress, uint8_t data)
{
	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte((uint8_t)(NRF24L01_W_REGISTER | regAddress));
	(void)NRF24L01_SPI_SwapByte(data);
	MySPI_Stop();
}

/* 连续写寄存器。 */
void NRF24L01_WriteRegs(uint8_t regAddress, const uint8_t *dataArray, uint8_t count)
{
	uint8_t i;

	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte((uint8_t)(NRF24L01_W_REGISTER | regAddress));
	for (i = 0U; i < count; i++)
	{
		(void)NRF24L01_SPI_SwapByte(dataArray[i]);
	}
	MySPI_Stop();
}

/* 读取 RX 载荷。 */
void NRF24L01_ReadRxPayload(uint8_t *dataArray, uint8_t count)
{
	uint8_t i;

	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte(NRF24L01_R_RX_PAYLOAD);
	for (i = 0U; i < count; i++)
	{
		dataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
	}
	MySPI_Stop();
}

/* 写入 TX 载荷。 */
void NRF24L01_WriteTxPayload(const uint8_t *dataArray, uint8_t count)
{
	uint8_t i;

	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte(NRF24L01_W_TX_PAYLOAD);
	for (i = 0U; i < count; i++)
	{
		(void)NRF24L01_SPI_SwapByte(dataArray[i]);
	}
	MySPI_Stop();
}

/* 清空 TX FIFO。 */
void NRF24L01_FlushTx(void)
{
	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_TX);
	MySPI_Stop();
}

/* 清空 RX FIFO。 */
void NRF24L01_FlushRx(void)
{
	NRF24L01_SelectSPI();
	MySPI_Start();
	(void)NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_RX);
	MySPI_Stop();
}

/* 读取状态寄存器。 */
uint8_t NRF24L01_ReadStatus(void)
{
	uint8_t status;

	NRF24L01_SelectSPI();
	MySPI_Start();
	status = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
	MySPI_Stop();

	return status;
}

/* 进入掉电模式（CE=0, PWR_UP=0）。 */
void NRF24L01_PowerDown(void)
{
	uint8_t config;

	NRF24L01_W_CE(0U);
	config = NRF24L01_ReadReg(NRF24L01_CONFIG);
	if (config == 0xFFU)
	{
		return;
	}
	config &= (uint8_t)(~0x02U);
	NRF24L01_WriteReg(NRF24L01_CONFIG, config);
}

/* 进入待机模式 I（CE=0, PWR_UP=1）。 */
void NRF24L01_StandbyI(void)
{
	uint8_t config;

	NRF24L01_W_CE(0U);
	config = NRF24L01_ReadReg(NRF24L01_CONFIG);
	if (config == 0xFFU)
	{
		return;
	}
	config |= 0x02U;
	NRF24L01_WriteReg(NRF24L01_CONFIG, config);
}

/* 进入接收模式（CE=1, PWR_UP=1, PRIM_RX=1）。 */
void NRF24L01_Rx(void)
{
	uint8_t config;

	NRF24L01_W_CE(0U);
	config = NRF24L01_ReadReg(NRF24L01_CONFIG);
	if (config == 0xFFU)
	{
		return;
	}
	config |= 0x03U;
	NRF24L01_WriteReg(NRF24L01_CONFIG, config);
	NRF24L01_W_CE(1U);
}

/* 进入发送模式（CE=1, PWR_UP=1, PRIM_RX=0）。 */
void NRF24L01_Tx(void)
{
	uint8_t config;

	NRF24L01_W_CE(0U);
	config = NRF24L01_ReadReg(NRF24L01_CONFIG);
	if (config == 0xFFU)
	{
		return;
	}
	config |= 0x02U;
	config &= (uint8_t)(~0x01U);
	NRF24L01_WriteReg(NRF24L01_CONFIG, config);
	NRF24L01_W_CE(1U);
}

/*
 * 初始化 NRF24L01：
 * - 按固定地址宽度/包长与常用无线参数写寄存器；
 * - 清 FIFO 与状态位后默认进入接收模式。
 */
void NRF24L01_Init(void)
{
	NRF24L01_GPIO_Init();

	NRF24L01_WriteReg(NRF24L01_CONFIG, 0x08U);
	NRF24L01_WriteReg(NRF24L01_EN_AA, 0x3FU);
	NRF24L01_WriteReg(NRF24L01_EN_RXADDR, 0x01U);
	NRF24L01_WriteReg(NRF24L01_SETUP_AW, 0x03U);
	NRF24L01_WriteReg(NRF24L01_SETUP_RETR, 0x03U);
	NRF24L01_WriteReg(NRF24L01_RF_CH, 0x02U);
	NRF24L01_WriteReg(NRF24L01_RF_SETUP, 0x0EU);

	NRF24L01_WriteReg(NRF24L01_RX_PW_P0, NRF24L01_RX_PACKET_WIDTH);
	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, NRF24L01_ADDR_WIDTH);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_STATUS, 0x70U);
	NRF24L01_Rx();
}

/*
 * 发送一个数据包：
 * 返回值：
 * 1=发送成功；2=达到最大重发；3=状态异常；4=超时。
 */
uint8_t NRF24L01_Send(void)
{
	uint8_t status;
	uint8_t sendFlag;
	uint32_t timeout;

	NRF24L01_WriteRegs(NRF24L01_TX_ADDR, NRF24L01_TxAddress, NRF24L01_ADDR_WIDTH);
	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_TxAddress, NRF24L01_ADDR_WIDTH);
	NRF24L01_WriteTxPayload(NRF24L01_TxPacket, NRF24L01_TX_PACKET_WIDTH);
	NRF24L01_Tx();

	timeout = 10000U;
	while (1)
	{
		status = NRF24L01_ReadStatus();
		timeout--;
		if (timeout == 0U)
		{
			sendFlag = 4U;
			NRF24L01_Init();
			break;
		}

		if ((status & 0x30U) == 0x30U)
		{
			sendFlag = 3U;
			NRF24L01_Init();
			break;
		}
		else if ((status & 0x10U) == 0x10U)
		{
			sendFlag = 2U;
			NRF24L01_Init();
			break;
		}
		else if ((status & 0x20U) == 0x20U)
		{
			sendFlag = 1U;
			break;
		}
	}

	NRF24L01_WriteReg(NRF24L01_STATUS, 0x30U);
	NRF24L01_FlushTx();
	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, NRF24L01_ADDR_WIDTH);
	NRF24L01_Rx();

	return sendFlag;
}

/*
 * 接收轮询：
 * 返回值：0=未收到；1=收到；2=状态异常；3=仍处于掉电模式。
 */
uint8_t NRF24L01_Receive(void)
{
	uint8_t status;
	uint8_t config;
	uint8_t receiveFlag;

	status = NRF24L01_ReadStatus();
	config = NRF24L01_ReadReg(NRF24L01_CONFIG);

	if ((config & 0x02U) == 0x00U)
	{
		receiveFlag = 3U;
		NRF24L01_Init();
	}
	else if ((status & 0x30U) == 0x30U)
	{
		receiveFlag = 2U;
		NRF24L01_Init();
	}
	else if ((status & 0x40U) == 0x40U)
	{
		receiveFlag = 1U;
		NRF24L01_ReadRxPayload(NRF24L01_RxPacket, NRF24L01_RX_PACKET_WIDTH);
		NRF24L01_WriteReg(NRF24L01_STATUS, 0x40U);
		NRF24L01_FlushRx();
	}
	else
	{
		receiveFlag = 0U;
	}

	return receiveFlag;
}

/* 运行时刷新接收地址（P0）。 */
void NRF24L01_UpdateRxAddress(void)
{
	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, NRF24L01_ADDR_WIDTH);
}

/* 最小通信测试：读写 SETUP_RETR 校验 SPI 与寄存器链路。 */
void App_NRF24L01_TestOnce(void)
{
	uint8_t value;

	NRF24L01_WriteReg(NRF24L01_SETUP_RETR, 0xA1U);
	value = NRF24L01_ReadReg(NRF24L01_SETUP_RETR);
	usart_printf(USART1, "[NRF24L01] SETUP_RETR=0x%02X\r\n", value);
}

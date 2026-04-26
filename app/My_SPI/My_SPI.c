#include "My_SPI.h"

#include "Delay.h"
#include "My_Usart/My_Usart.h"

/*
 * 软件 SPI 板级资源注册表：
 * 当前先按单路使用，默认取第 1 个配置项。
 */
static const MySPI_Config_t *s_spiTable;
static uint8_t s_spiCount;
static volatile MySPI_BusId_t s_activeBusId = My_SPI1;
static volatile SPI_SpeedTypeDef s_spiSpeed = SPI_SPEED_500K;

/*
 * 按当前 SPI 速率档位插入一个基础延时。
 * 500K 档相对保守，1M 档将延时减半。
 */
static void MySPI_DelayBySpeed(void)
{
	if (s_spiSpeed == SPI_SPEED_5M)
	{
		/* 5M 档不额外插入软件延时，由 GPIO 翻转与函数开销决定极限速率。 */
	}
	else if (s_spiSpeed == SPI_SPEED_1M)
	{
		Delay_us(1U);
	}
	else
	{
		Delay_us(2U);
	}
}

/*
 * 获取当前软件 SPI 使用的板级配置。
 * 返回 0 表示还未调用 MySPI_Register 注册映射。
 */
static const MySPI_Config_t *MySPI_GetConfigById(MySPI_BusId_t busId)
{
	uint8_t i;

	if ((s_spiTable == 0) || (s_spiCount == 0U))
	{
		return 0;
	}

	for (i = 0U; i < s_spiCount; i++)
	{
		if (s_spiTable[i].id == (uint8_t)busId)
		{
			return &s_spiTable[i];
		}
	}

	return 0;
}

static const MySPI_Config_t *MySPI_GetConfig(void)
{
	return MySPI_GetConfigById(s_activeBusId);
}

/*
 * 注册板级 SPI 资源表。
 * 由 Enroll_SPI_Register 在系统初始化阶段调用。
 */
void MySPI_Register(const MySPI_Config_t *configTable, uint8_t count)
{
	s_spiTable = configTable;
	s_spiCount = count;
	if ((configTable != 0) && (count > 0U))
	{
		s_activeBusId = (MySPI_BusId_t)configTable[0].id;
	}
	else
	{
		s_activeBusId = My_SPI1;
	}
	s_spiSpeed = SPI_SPEED_500K;
}

void MySPI_SelectBus(MySPI_BusId_t busId)
{
	if (MySPI_GetConfigById(busId) != 0)
	{
		s_activeBusId = busId;
	}
}

/* 设置软件 SPI 速率档位。 */
void MySPI_SetSpeed(SPI_SpeedTypeDef speed)
{
	if (speed == SPI_SPEED_5M)
	{
		s_spiSpeed = SPI_SPEED_5M;
	}
	else if (speed == SPI_SPEED_1M)
	{
		s_spiSpeed = SPI_SPEED_1M;
	}
	else
	{
		s_spiSpeed = SPI_SPEED_500K;
	}
}

/* 获取当前软件 SPI 速率档位。 */
SPI_SpeedTypeDef MySPI_GetSpeed(void)
{
	return s_spiSpeed;
}

/*
 * 写 CS 电平：
 * bitValue=0 选中从机，bitValue=1 释放从机。
 */
void MySPI_W_SS(uint8_t bitValue)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_Write(config->csPort, config->csPin, bitValue);
}

/* 写 SCK 电平。 */
void MySPI_W_SCK(uint8_t bitValue)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_Write(config->sckPort, config->sckPin, bitValue);
}

/* 写 MOSI 电平。 */
void MySPI_W_MOSI(uint8_t bitValue)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_Write(config->mosiPort, config->mosiPin, bitValue);
}

/* 读 MISO 电平。 */
uint8_t MySPI_R_MISO(void)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return 0U;
	}

	return API_GPIO_Read(config->misoPort, config->misoPin);
}

/*
 * 初始化软件 SPI：
 * 1) CS/SCK/MOSI 配置为输出，MISO 配置为上拉输入
 * 2) 置 SPI 模式0空闲态（CS=1, SCK=0）
 */
void MySPI_Init(void)
{
	const MySPI_Config_t *config;
    uint8_t i;

	if ((s_spiTable == 0) || (s_spiCount == 0U))
	{
		return;
	}

	for (i = 0U; i < s_spiCount; i++)
	{
		config = &s_spiTable[i];
		API_GPIO_InitOutput(config->csPort, config->csPin);
		API_GPIO_InitOutput(config->sckPort, config->sckPin);
		API_GPIO_InitOutput(config->mosiPort, config->mosiPin);
		API_GPIO_InitInputPullUp(config->misoPort, config->misoPin);
		API_GPIO_Write(config->csPort, config->csPin, 1U);
		API_GPIO_Write(config->sckPort, config->sckPin, 0U);
	}

	/* 模式0默认空闲：CS 高、SCK 低。 */
	MySPI_W_SS(1U);
	MySPI_W_SCK(0U);
}

/* SPI 起始：拉低 CS。 */
void MySPI_Start(void)
{
	MySPI_W_SS(0U);
}

/* SPI 终止：拉高 CS。 */
void MySPI_Stop(void)
{
	MySPI_W_SS(1U);
}

/*
 * 交换传输 1 字节（SPI 模式0）：
 * - 上升沿采样 MISO，下降沿准备下一位 MOSI。
 */
uint8_t MySPI_SwapByte(uint8_t byteSend)
{
	uint8_t i;
	uint8_t byteReceive;

	byteReceive = 0x00U;

	for (i = 0U; i < 8U; i++)
	{
		MySPI_W_MOSI((uint8_t)!!(byteSend & (uint8_t)(0x80U >> i)));
		MySPI_DelayBySpeed();
		MySPI_W_SCK(1U);
		MySPI_DelayBySpeed();
		if (MySPI_R_MISO() != 0U)
		{
			byteReceive |= (uint8_t)(0x80U >> i);
		}
		MySPI_DelayBySpeed();
		MySPI_W_SCK(0U);
		MySPI_DelayBySpeed();
	}

	return byteReceive;
}

/*
 * 最小 SPI 测试例程：
 * - 连续发送 5 个测试字节并打印收发值。
 * - 若 MOSI 与 MISO 回环短接，理论上 RX 与 TX 一致。
 */
void App_SPI_TestOnce(void)
{
	static const uint8_t s_txData[] = {0x9AU, 0x55U, 0xA5U, 0x00U, 0xFFU};
	uint8_t i;
	uint8_t rx;

	if (MySPI_GetConfig() == 0)
	{
		usart_printf(USART1, "\r\n[SPI] test skipped: no bus registered\r\n");
		return;
	}

	usart_printf(USART1, "\r\n[SPI] test start\r\n");
	MySPI_Start();
	for (i = 0U; i < (uint8_t)(sizeof(s_txData) / sizeof(s_txData[0])); i++)
	{
		rx = MySPI_SwapByte(s_txData[i]);
		usart_printf(USART1, "[SPI] TX=0x%02X RX=0x%02X\r\n", s_txData[i], rx);
	}
	MySPI_Stop();
	usart_printf(USART1, "[SPI] test done\r\n");
}

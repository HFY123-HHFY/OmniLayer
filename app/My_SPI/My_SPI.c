#include "My_SPI.h"

#include "Delay.h"
#include "My_Usart/My_Usart.h"

/*
 * 软件 SPI 板级资源注册表：
 * 当前先按单路使用，默认取第 1 个配置项。
 */
static const MySPI_Config_t *s_spiTable;
static uint8_t s_spiCount;

static const MySPI_Config_t *MySPI_GetConfig(void)
{
	if ((s_spiTable == 0) || (s_spiCount == 0U))
	{
		return 0;
	}

	return &s_spiTable[0];
}

void MySPI_Register(const MySPI_Config_t *configTable, uint8_t count)
{
	s_spiTable = configTable;
	s_spiCount = count;
}

void MySPI_W_SS(uint8_t bitValue)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_Write(config->port, config->csPin, bitValue);
}

void MySPI_W_SCK(uint8_t bitValue)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_Write(config->port, config->sckPin, bitValue);
}

void MySPI_W_MOSI(uint8_t bitValue)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_Write(config->port, config->mosiPin, bitValue);
}

uint8_t MySPI_R_MISO(void)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return 0U;
	}

	return API_GPIO_Read(config->port, config->misoPin);
}

void MySPI_Init(void)
{
	const MySPI_Config_t *config;

	config = MySPI_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_InitOutput(config->port, config->csPin);
	API_GPIO_InitOutput(config->port, config->sckPin);
	API_GPIO_InitOutput(config->port, config->mosiPin);
	API_GPIO_InitInputPullUp(config->port, config->misoPin);

	/* 模式0默认空闲：CS 高、SCK 低。 */
	MySPI_W_SS(1U);
	MySPI_W_SCK(0U);
}

void MySPI_Start(void)
{
	MySPI_W_SS(0U);
}

void MySPI_Stop(void)
{
	MySPI_W_SS(1U);
}

uint8_t MySPI_SwapByte(uint8_t byteSend)
{
	uint8_t i;
	uint8_t byteReceive;

	byteReceive = 0x00U;

	for (i = 0U; i < 8U; i++)
	{
		MySPI_W_MOSI((uint8_t)!!(byteSend & (uint8_t)(0x80U >> i)));
		Delay_us(1U);
		MySPI_W_SCK(1U);
		if (MySPI_R_MISO() != 0U)
		{
			byteReceive |= (uint8_t)(0x80U >> i);
		}
		Delay_us(1U);
		MySPI_W_SCK(0U);
		Delay_us(1U);
	}

	return byteReceive;
}

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

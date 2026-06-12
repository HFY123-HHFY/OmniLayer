#include "API_SPI.h"
#include "soft_spi_hal.h"

#include "Delay.h"
#include "My_Usart/My_Usart.h"

/* ===================== 模块状态变量 ===================== */

static const API_SPI_Config_t *s_spiTable;
static uint8_t s_spiCount;
static volatile API_SPI_BusId_t s_activeBusId = API_SPI1;
static volatile API_SPI_SpeedTypeDef s_spiSpeed = API_SPI_SPEED_500K;

/* ===================== 内部辅助 ===================== */

/*
 * 按 busId 查找总线配置表。
 */
static const API_SPI_Config_t *API_SPI_GetConfigById(API_SPI_BusId_t busId)
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

/* ===================== 公共 API ===================== */

/*
 * 注册板级 SPI 资源表。
 */
void API_SPI_Register(const API_SPI_Config_t *configTable, uint8_t count)
{
	s_spiTable = configTable;
	s_spiCount = count;

	if ((configTable != 0) && (count > 0U))
	{
		s_activeBusId = (API_SPI_BusId_t)configTable[0].id;
		soft_spi_hal_init(configTable[0].csPort, configTable[0].csPin, configTable[0].csIomux,
		                  configTable[0].sckPort, configTable[0].sckPin, configTable[0].sckIomux,
		                  configTable[0].mosiPort, configTable[0].mosiPin, configTable[0].mosiIomux,
		                  configTable[0].misoPort, configTable[0].misoPin, configTable[0].misoIomux);
	}
	else
	{
		s_activeBusId = API_SPI1;
	}

	s_spiSpeed = API_SPI_SPEED_500K;
	soft_spi_hal_set_speed(500U);
}

/*
 * 选择当前操作的软件 SPI 总线。
 */
void API_SPI_SelectBus(API_SPI_BusId_t busId)
{
	const API_SPI_Config_t *cfg;

	cfg = API_SPI_GetConfigById(busId);
	if (cfg != 0)
	{
		s_activeBusId = busId;
		soft_spi_hal_init(cfg->csPort, cfg->csPin, cfg->csIomux,
		                  cfg->sckPort, cfg->sckPin, cfg->sckIomux,
		                  cfg->mosiPort, cfg->mosiPin, cfg->mosiIomux,
		                  cfg->misoPort, cfg->misoPin, cfg->misoIomux);
	}

	/* 总线切换后恢复默认延时 */
	soft_spi_hal_delay_on();
}

/*
 * 关闭 SPI bit-bang 延时 (OLED 等高速设备全速运行)。
 * 下一次 API_SPI_SelectBus 会自动恢复延时。
 */
void API_SPI_DelayOff(void)
{
	soft_spi_hal_delay_off();
}

/*
 * 恢复 SPI bit-bang 延时。
 */
void API_SPI_DelayOn(void)
{
	soft_spi_hal_delay_on();
}

/*
 * 设置软件 SPI 速率档位。
 */
void API_SPI_SetSpeed(API_SPI_SpeedTypeDef speed)
{
	switch (speed)
	{
	case API_SPI_SPEED_5M:
		s_spiSpeed = API_SPI_SPEED_5M;
		soft_spi_hal_set_speed(5000U);
		break;
	case API_SPI_SPEED_2M:
		s_spiSpeed = API_SPI_SPEED_2M;
		soft_spi_hal_set_speed(2000U);
		break;
	case API_SPI_SPEED_1M:
		s_spiSpeed = API_SPI_SPEED_1M;
		soft_spi_hal_set_speed(1000U);
		break;
	case API_SPI_SPEED_250K:
		s_spiSpeed = API_SPI_SPEED_250K;
		soft_spi_hal_set_speed(250U);
		break;
	default: /* 500K */
		s_spiSpeed = API_SPI_SPEED_500K;
		soft_spi_hal_set_speed(500U);
		break;
	}
}

/*
 * 获取当前软件 SPI 速率档位。
 */
API_SPI_SpeedTypeDef API_SPI_GetSpeed(void)
{
	return s_spiSpeed;
}

/* ===================== 初始化 ===================== */

/*
 * 初始化所有已注册的软件 SPI 总线的引脚模式:
 * - CS/SCK/MOSI → 推挽输出
 * - MISO → 上拉输入
 * - 模式 0 空闲态: CS=1, SCK=0
 */
void API_SPI_Init(void)
{
	uint8_t i;
	API_SPI_BusId_t prevBus;

	if ((s_spiTable == 0) || (s_spiCount == 0U))
	{
		return;
	}

	/* 保存当前活跃总线，防止下面的遍历覆盖掉寄存器指针 */
	prevBus = s_activeBusId;

	for (i = 0U; i < s_spiCount; i++)
	{
		const API_SPI_Config_t *cfg = &s_spiTable[i];
		soft_spi_hal_init(cfg->csPort, cfg->csPin, cfg->csIomux,
		                  cfg->sckPort, cfg->sckPin, cfg->sckIomux,
		                  cfg->mosiPort, cfg->mosiPin, cfg->mosiIomux,
		                  cfg->misoPort, cfg->misoPin, cfg->misoIomux);
	}

	/* 恢复活跃总线的寄存器指针 */
	{
		const API_SPI_Config_t *cfg = API_SPI_GetConfigById(prevBus);
		if (cfg != 0)
		{
			soft_spi_hal_init(cfg->csPort, cfg->csPin, cfg->csIomux,
			                  cfg->sckPort, cfg->sckPin, cfg->sckIomux,
			                  cfg->mosiPort, cfg->mosiPin, cfg->mosiIomux,
			                  cfg->misoPort, cfg->misoPin, cfg->misoIomux);
		}
	}

	/* 模式 0 空闲态 */
	soft_spi_hal_w_cs(1U);
	soft_spi_hal_w_sck(0U);
}

/* ===================== 协议层 ===================== */

/*
 * SPI 起始: 拉低 CS。
 */
void API_SPI_Start(void)
{
	soft_spi_hal_w_cs(0U);
}

/*
 * SPI 终止: 拉高 CS。
 */
void API_SPI_Stop(void)
{
	soft_spi_hal_w_cs(1U);
}

/*
 * 交换传输 1 字节 (SPI 模式 0):
 * - 上升沿采样 MISO, 下降沿准备下一位 MOSI。
 */
uint8_t API_SPI_SwapByte(uint8_t byteSend)
{
	uint8_t i;
	uint8_t byteReceive;

	byteReceive = 0x00U;

	for (i = 0U; i < 8U; i++)
	{
		/* 设置 MOSI (MSB first) */
		soft_spi_hal_w_mosi((uint8_t)((byteSend & (0x80U >> i)) != 0U ? 1U : 0U));

		soft_spi_hal_delay_us(1U);

		/* SCK 上升沿 → 从机输出数据, 主机采样 */
		soft_spi_hal_w_sck(1U);

		soft_spi_hal_delay_us(1U);

		/* 采样 MISO */
		if (soft_spi_hal_r_miso() != 0U)
		{
			byteReceive |= (uint8_t)(0x80U >> i);
		}

		soft_spi_hal_delay_us(1U);

		/* SCK 下降沿 → 准备下一位 */
		soft_spi_hal_w_sck(0U);

		soft_spi_hal_delay_us(1U);
	}

	return byteReceive;
}

/* ===================== SPI 测试 (调试用) ===================== */

/*
 * 最小 SPI 测试例程:
 * - 连续发送 5 个测试字节并打印收发值。
 */
void App_SPI_TestOnce(void)
{
	static const uint8_t s_txData[] = {0x9AU, 0x55U, 0xA5U, 0x00U, 0xFFU};
	uint8_t i;
	uint8_t rx;

	if (s_spiTable == 0 || s_spiCount == 0U)
	{
		usart_printf(USART1, "\r\n[SPI] test skipped: no bus registered\r\n");
		return;
	}

	usart_printf(USART1, "\r\n[SPI] test start\r\n");
	API_SPI_Start();
	for (i = 0U; i < (uint8_t)(sizeof(s_txData) / sizeof(s_txData[0])); i++)
	{
		rx = API_SPI_SwapByte(s_txData[i]);
		usart_printf(USART1, "[SPI] TX=0x%02X RX=0x%02X\r\n", s_txData[i], rx);
	}
	API_SPI_Stop();
	usart_printf(USART1, "[SPI] test done\r\n");
}

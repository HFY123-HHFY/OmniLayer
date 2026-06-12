#include "API_I2C.h"
#include "soft_i2c_hal.h"

#include "Delay.h"
#include "My_Usart/My_Usart.h"

/* ===================== 模块状态变量 ===================== */

/*
 * 注册表: 保存 Enroll 层下发的所有总线映射。
 */
static const API_I2C_Config_t *s_i2cTable;
static uint8_t s_i2cCount;
static volatile API_I2C_BusId_t s_activeBusId = API_I2C1;
static volatile API_I2C_SpeedTypeDef s_i2cSpeed = API_I2C_SPEED_100K;

/* ===================== 内部辅助 ===================== */

/*
 * 按 busId 查找已注册的总线配置表。
 */
static const API_I2C_Config_t *API_I2C_GetConfigById(API_I2C_BusId_t busId)
{
	uint8_t i;

	if ((s_i2cTable == 0) || (s_i2cCount == 0U))
	{
		return 0;
	}

	for (i = 0U; i < s_i2cCount; i++)
	{
		if (s_i2cTable[i].id == (uint8_t)busId)
		{
			return &s_i2cTable[i];
		}
	}

	return 0;
}

/* ===================== 公共 API ===================== */

/*
 * 注册板级 I2C 资源表。
 */
void API_I2C_Register(const API_I2C_Config_t *configTable, uint8_t count)
{
	s_i2cTable = configTable;
	s_i2cCount = count;

	if ((configTable != 0) && (count > 0U))
	{
		s_activeBusId = (API_I2C_BusId_t)configTable[0].id;
		soft_i2c_hal_init(configTable[0].sclPort, configTable[0].sclPin, configTable[0].sclIomux,
		                  configTable[0].sdaPort, configTable[0].sdaPin, configTable[0].sdaIomux);
	}
	else
	{
		s_activeBusId = API_I2C1;
	}
}

/*
 * 选择当前操作的软件 I2C 总线。
 */
void API_I2C_SelectBus(API_I2C_BusId_t busId)
{
	const API_I2C_Config_t *cfg;

	cfg = API_I2C_GetConfigById(busId);
	if (cfg != 0)
	{
		s_activeBusId = busId;
		soft_i2c_hal_init(cfg->sclPort, cfg->sclPin, cfg->sclIomux,
		                  cfg->sdaPort, cfg->sdaPin, cfg->sdaIomux);
	}

	/* 总线切换后恢复默认延时 (其他设备如 MPU6050 需要协议合规的时序) */
	soft_i2c_hal_delay_on();
}

/*
 * 关闭 bit-bang 延时 (OLED 等高速设备全速运行)。
 * 注意: 下一次 API_I2C_SelectBus 会自动恢复延时。
 */
void API_I2C_DelayOff(void)
{
	soft_i2c_hal_delay_off();
}

/*
 * 恢复 bit-bang 延时。
 */
void API_I2C_DelayOn(void)
{
	soft_i2c_hal_delay_on();
}

/*
 * 设置软件 I2C 速率档位。
 */
void API_I2C_SetSpeed(API_I2C_SpeedTypeDef speed)
{
	switch (speed)
	{
	case API_I2C_SPEED_400K:
		s_i2cSpeed = API_I2C_SPEED_400K;
		soft_i2c_hal_set_speed(400U);
		break;
	case API_I2C_SPEED_200K:
		s_i2cSpeed = API_I2C_SPEED_200K;
		soft_i2c_hal_set_speed(200U);
		break;
	case API_I2C_SPEED_50K:
		s_i2cSpeed = API_I2C_SPEED_50K;
		soft_i2c_hal_set_speed(50U);
		break;
	default:
		s_i2cSpeed = API_I2C_SPEED_100K;
		soft_i2c_hal_set_speed(100U);
		break;
	}
}

/*
 * 获取当前软件 I2C 速率档位。
 */
API_I2C_SpeedTypeDef API_I2C_GetSpeed(void)
{
	return s_i2cSpeed;
}

/* ===================== 初始化 ===================== */

/*
 * 初始化所有已注册的软件 I2C 总线:
 * - 配置 SCL/SDA 为推挽输出 (开漏仿真)
 * - 拉高 SCL/SDA 进入 I2C 空闲态
 */
void API_I2C_Init(void)
{
	uint8_t i;
	API_I2C_BusId_t prevBus;

	if ((s_i2cTable == 0) || (s_i2cCount == 0U))
	{
		return;
	}

	/* 保存当前活跃总线，防止下面的遍历覆盖掉寄存器指针 */
	prevBus = s_activeBusId;

	for (i = 0U; i < s_i2cCount; i++)
	{
		const API_I2C_Config_t *cfg = &s_i2cTable[i];
		soft_i2c_hal_init(cfg->sclPort, cfg->sclPin, cfg->sclIomux,
		                  cfg->sdaPort, cfg->sdaPin, cfg->sdaIomux);
	}

	/* 恢复活跃总线的寄存器指针 */
	{
		const API_I2C_Config_t *cfg = API_I2C_GetConfigById(prevBus);
		if (cfg != 0)
		{
			soft_i2c_hal_init(cfg->sclPort, cfg->sclPin, cfg->sclIomux,
			                  cfg->sdaPort, cfg->sdaPin, cfg->sdaIomux);
		}
	}

	/* 释放总线到空闲态 */
	soft_i2c_hal_set_sda_output();
	soft_i2c_hal_w_scl(1U);
	soft_i2c_hal_w_sda(1U);

	/* 恢复默认速率延时 */
	soft_i2c_hal_set_speed(100U);
	soft_i2c_hal_delay_on();
}

/* ===================== 协议层 ===================== */

/*
 * I2C 起始条件:
 * SCL 高电平期间, SDA 从高跳变到低。
 */
void API_I2C_Start(void)
{
	soft_i2c_hal_set_sda_output();
	soft_i2c_hal_w_sda(1U);
	soft_i2c_hal_w_scl(1U);
	soft_i2c_hal_delay_us(4U);
	soft_i2c_hal_w_sda(0U);
	soft_i2c_hal_delay_us(4U);
	soft_i2c_hal_w_scl(0U); /* 钳住总线, 准备发送 */
}

/*
 * I2C 停止条件:
 * SCL 高电平期间, SDA 从低跳变到高。
 */
void API_I2C_Stop(void)
{
	soft_i2c_hal_set_sda_output();
	soft_i2c_hal_w_scl(0U);
	soft_i2c_hal_w_sda(0U);
	soft_i2c_hal_delay_us(4U);
	soft_i2c_hal_w_scl(1U);
	soft_i2c_hal_w_sda(1U);
	soft_i2c_hal_delay_us(4U);
}

/*
 * 等待从机 ACK:
 * 返回 0 = 收到 ACK (SDA 被拉低), 1 = 超时 / NACK。
 */
uint8_t API_I2C_Wait_Ack(void)
{
	uint16_t ErrTime = 0U;

	soft_i2c_hal_set_sda_input();
	soft_i2c_hal_delay_us(1U);
	soft_i2c_hal_w_scl(1U);
	soft_i2c_hal_delay_us(1U);

	while (soft_i2c_hal_r_sda())
	{
		ErrTime++;
		if (ErrTime > API_I2C_ACK_TIMEOUT_COUNT)
		{
			API_I2C_Stop();
			return 1U;
		}
	}

	soft_i2c_hal_w_scl(0U);
	return 0U;
}

/*
 * 发送 ACK: 第 9 个时钟拉低 SDA。
 */
void API_I2C_Ack(void)
{
	soft_i2c_hal_w_scl(0U);
	soft_i2c_hal_set_sda_output();
	soft_i2c_hal_w_sda(0U);
	soft_i2c_hal_delay_us(2U);
	soft_i2c_hal_w_scl(1U);
	soft_i2c_hal_delay_us(2U);
	soft_i2c_hal_w_scl(0U);
}

/*
 * 发送 NACK: 第 9 个时钟保持 SDA 高。
 */
void API_I2C_NAck(void)
{
	soft_i2c_hal_w_scl(0U);
	soft_i2c_hal_set_sda_output();
	soft_i2c_hal_w_sda(1U);
	soft_i2c_hal_delay_us(2U);
	soft_i2c_hal_w_scl(1U);
	soft_i2c_hal_delay_us(2U);
	soft_i2c_hal_w_scl(0U);
}

/*
 * 发送 1 个字节 (MSB first)。
 */
void API_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;

	soft_i2c_hal_set_sda_output();
	for (i = 0U; i < 8U; i++)
	{
		soft_i2c_hal_w_sda((Byte & 0x80U) >> 7U);
		Byte <<= 1U;
		soft_i2c_hal_delay_us(2U);
		soft_i2c_hal_w_scl(1U);
		soft_i2c_hal_delay_us(2U);
		soft_i2c_hal_w_scl(0U);
		soft_i2c_hal_delay_us(2U);
	}
}

/*
 * 接收 1 个字节:
 * Ack = 1 发送 ACK, Ack = 0 发送 NACK。
 */
uint8_t API_I2C_ReceiveByte(unsigned char Ack)
{
	unsigned char i;
	unsigned char Byte = 0U;

	soft_i2c_hal_set_sda_input();
	for (i = 0U; i < 8U; i++)
	{
		soft_i2c_hal_w_scl(0U);
		soft_i2c_hal_delay_us(2U);
		soft_i2c_hal_w_scl(1U);
		Byte <<= 1U;
		if (soft_i2c_hal_r_sda() != 0U)
		{
			Byte++;
		}
		soft_i2c_hal_delay_us(1U);
	}

	if (Ack != 0U)
	{
		API_I2C_Ack();
	}
	else
	{
		API_I2C_NAck();
	}

	return Byte;
}

/* ===================== I2C 扫描 (调试用) ===================== */

/*
 * 扫描指定总线并输出在线设备地址。
 */
static void App_I2C_ScanBus(API_I2C_BusId_t busId)
{
	uint8_t addr;
	uint8_t foundCount;
	const API_I2C_Config_t *config;
	API_I2C_BusId_t prevBusId;
	API_I2C_SpeedTypeDef prevSpeed;

	config = API_I2C_GetConfigById(busId);
	if (config == 0)
	{
		return;
	}

	prevBusId = s_activeBusId;
	prevSpeed = s_i2cSpeed;

	API_I2C_SelectBus(busId);
	API_I2C_SetSpeed(API_I2C_SPEED_100K);

	foundCount = 0U;
	usart_printf(USART1, "\r\n[I2C][API_I2C%u] scan start\r\n", (unsigned int)((uint8_t)busId + 1U));

	for (addr = 1U; addr < 0x7FU; addr++)
	{
		API_I2C_Start();
		API_I2C_SendByte((uint8_t)(addr << 1));
		if (API_I2C_Wait_Ack() == 0U)
		{
			foundCount++;
			usart_printf(USART1, "[I2C][API_I2C%u] found: 0x%02X\r\n", (unsigned int)((uint8_t)busId + 1U), addr);
		}
		API_I2C_Stop();
		Delay_ms(1U);
	}

	usart_printf(USART1, "[I2C][API_I2C%u] scan done, count=%u\r\n", (unsigned int)((uint8_t)busId + 1U), foundCount);

	API_I2C_SelectBus(prevBusId);
	API_I2C_SetSpeed(prevSpeed);
}

/*
 * 遍历所有已注册总线并执行 I2C 扫描。
 */
void App_I2C_ScanOnce(void)
{
	uint8_t i;

	if ((s_i2cTable == 0) || (s_i2cCount == 0U))
	{
		usart_printf(USART1, "\r\n[I2C] scan skipped: no bus registered\r\n");
		return;
	}

	for (i = 0U; i < s_i2cCount; i++)
	{
		App_I2C_ScanBus((API_I2C_BusId_t)s_i2cTable[i].id);
	}
}

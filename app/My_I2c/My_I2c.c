#include "My_I2c.h"

#include "Delay.h"
#include "My_Usart/My_Usart.h"
/*
 * 软件 I2C 的板级资源注册表：
 * - s_i2cTable/s_i2cCount 保存注册层下发的所有总线映射。
 * - s_activeBusId 表示当前协议收发所使用的总线。
 * - s_i2cSpeed 是全局速率档位，驱动在事务前可按需切换。
 */
static const MyI2C_Config_t *s_i2cTable;
static uint8_t s_i2cCount;
static volatile MyI2C_BusId_t s_activeBusId = My_I2C1;
static volatile I2C_SpeedTypeDef s_i2cSpeed = I2C_SPEED_100K;

/*
 * I2C 开漏等效驱动：
 * - 输出 0：配置为输出并拉低
 * - 输出 1：配置为上拉输入，释放总线
 */
static void MyI2C_DriveLine(void *port, uint16_t pin, uint8_t level)
{
	if (level != 0U)
	{
		API_GPIO_InitInputPullUp(port, pin);
	}
	else
	{
		API_GPIO_InitOutput(port, pin);
		API_GPIO_Write(port, pin, 0U);
	}
}

/*
 * 按当前速率档位，把“100k 档基准延时”换算成实际延时。
 * 例如：baseUs=5 时，100k 档约为 5us，400k 档约为 1us。
 */
static void MyI2C_DelayByBaseUs(uint8_t baseUs)
{
	uint16_t delayUs;

	if (s_i2cSpeed == I2C_SPEED_400K)
	{
		delayUs = (uint16_t)(((uint16_t)baseUs * I2C_DELAY_400K + I2C_DELAY_100K - 1U) / I2C_DELAY_100K);
	}
	else if (s_i2cSpeed == I2C_SPEED_200K)
	{
		delayUs = (uint16_t)(((uint16_t)baseUs * I2C_DELAY_200K + I2C_DELAY_100K - 1U) / I2C_DELAY_100K);
	}
	else if (s_i2cSpeed == I2C_SPEED_50K)
	{
		delayUs = (uint16_t)(((uint16_t)baseUs * I2C_DELAY_50K + I2C_DELAY_100K - 1U) / I2C_DELAY_100K);
	}
	else
	{
		delayUs = baseUs;
	}

	if (delayUs == 0U)
	{
		delayUs = 1U;
	}

	Delay_us(delayUs);
}

/*
 * 获取当前软件 I2C 使用的板级配置。
 * 返回 0 表示还未调用 MyI2C_Register 注册映射。
 */
static const MyI2C_Config_t *MyI2C_GetConfigById(MyI2C_BusId_t busId)
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

/* 获取当前选中总线的软件 I2C 配置。 */
static const MyI2C_Config_t *MyI2C_GetConfig(void)
{
	return MyI2C_GetConfigById(s_activeBusId);
}

/*
 * 注册板级 I2C 资源表。
 * 由 Enroll_I2C_Register 在系统初始化阶段调用。
 */
void MyI2C_Register(const MyI2C_Config_t *configTable, uint8_t count)
{
	s_i2cTable = configTable;
	s_i2cCount = count;

	if ((configTable != 0) && (count > 0U))
	{
		s_activeBusId = (MyI2C_BusId_t)configTable[0].id;
	}
	else
	{
		s_activeBusId = My_I2C1;
	}
}

/** 函    数：选择I2C总线 */
void MyI2C_SelectBus(MyI2C_BusId_t busId)
{
	if (MyI2C_GetConfigById(busId) != 0)
	{
		s_activeBusId = busId;
	}
}

/** 函    数：设置I2C速率 */
void MyI2C_SetSpeed(I2C_SpeedTypeDef speed)
{
	if (speed == I2C_SPEED_400K)
	{
		s_i2cSpeed = I2C_SPEED_400K;
	}
	else if (speed == I2C_SPEED_200K)
	{
		s_i2cSpeed = I2C_SPEED_200K;
	}
	else if (speed == I2C_SPEED_50K)
	{
		s_i2cSpeed = I2C_SPEED_50K;
	}
	else
	{
		s_i2cSpeed = I2C_SPEED_100K;
	}
}

/* 获取当前软件 I2C 速率档位*/
I2C_SpeedTypeDef MyI2C_GetSpeed(void)
{
	return s_i2cSpeed;
}

/*
 * 初始化软件 I2C：
 * 1) SCL/SDA 配置为输出
 * 2) 拉高 SCL/SDA，进入 I2C 空闲态
 */
void MyI2C_Init(void)
{
	const MyI2C_Config_t *config;
	uint8_t i;

	if ((s_i2cTable == 0) || (s_i2cCount == 0U))
	{
		return;
	}

	for (i = 0U; i < s_i2cCount; i++)
	{
		config = &s_i2cTable[i];
		/* I2C 空闲态：SCL=1，SDA=1（释放两条线） */
		MyI2C_DriveLine(config->port, config->sclPin, 1U);
		MyI2C_DriveLine(config->port, config->sdaPin, 1U);
	}
}

/*
 * 写 SCL 电平：
 * 电平变化后保留 5us 延时，确保协议时序稳定。
 */
void MyI2C_W_SCL(uint8_t BitValue) // 写SCL
{
	const MyI2C_Config_t *config;

	config = MyI2C_GetConfig();
	if (config == 0)
	{
		return;
	}

	MyI2C_DriveLine(config->port, config->sclPin, BitValue);
	MyI2C_DelayByBaseUs(5U);
}

/*
 * 写 SDA 电平：
 * 用于起始、停止和写数据位等阶段。
 */
void MyI2C_W_SDA(uint8_t BitValue) // 写SDA
{
	const MyI2C_Config_t *config;

	config = MyI2C_GetConfig();
	if (config == 0)
	{
		return;
	}

	MyI2C_DriveLine(config->port, config->sdaPin, BitValue);
	MyI2C_DelayByBaseUs(5U);
}

/*
 * 读取 SDA 电平：
 * 用于 ACK 检测和接收数据位。
 */
uint8_t MyI2C_R_SDA(void) // 读SDA
{
	const MyI2C_Config_t *config;

	config = MyI2C_GetConfig();
	if (config == 0)
	{
		return 1U;
	}

	MyI2C_DelayByBaseUs(5U);
	return API_GPIO_Read(config->port, config->sdaPin);
}

/*
 * 将 SDA 配置为输入：
 * 在等待从机 ACK、接收数据前调用。
 */
void MyI2C_Set_SDA_Input(void) // 设置SDA为输入模式
{
	const MyI2C_Config_t *config;

	config = MyI2C_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_InitInputPullUp(config->port, config->sdaPin);
}

/*
 * 将 SDA 配置为输出：
 * 在发送起始、停止和写数据位前调用。
 */
void MyI2C_Set_SDA_Output(void) // 设置SDA为输出模式
{
	const MyI2C_Config_t *config;

	config = MyI2C_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_InitOutput(config->port, config->sdaPin);
}

/*
 * 协议层：起始条件。
 * 条件：SCL 高电平期间，SDA 从高跳变到低。
 */
void MyI2C_Start(void) // I2C起始
{
	MyI2C_Set_SDA_Output();
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	MyI2C_DelayByBaseUs(4U);
	MyI2C_W_SDA(0);
	MyI2C_DelayByBaseUs(4U);
	MyI2C_W_SCL(0);//钳住I2C总线，准备发送或接收数据 
}

/*
 * 协议层：停止条件。
 * 条件：SCL 高电平期间，SDA 从低跳变到高。
 */
void MyI2C_Stop(void) // I2C停止
{
    MyI2C_Set_SDA_Output();
    MyI2C_W_SCL(0);
    MyI2C_W_SDA(0);// STOP: 当SCL为高时，SDA从低变高
	MyI2C_DelayByBaseUs(4U);
    MyI2C_W_SCL(1);
    MyI2C_W_SDA(1);
	MyI2C_DelayByBaseUs(4U);
}

/*
 * 等待从机 ACK：
 * 返回值 1 表示超时失败，0 表示收到应答。
 * 这里保留你原先的超时策略和时序，不改协议。
 */
uint8_t MyI2C_Wait_Ack(void) // 等待应答信号到来
{
	uint8_t ErrTime = 0;
	MyI2C_Set_SDA_Input();
	MyI2C_W_SDA(1);
	MyI2C_DelayByBaseUs(1U);
	MyI2C_W_SCL(1);
	MyI2C_DelayByBaseUs(1U);
	while (MyI2C_R_SDA())
	{
		ErrTime++;
		if (ErrTime > 250)
		{
			MyI2C_Stop();
			return 1; // 超时未响应
		}
	}
	MyI2C_W_SCL(0);
	return 0; // 应答成功
}

/*
 * 发送 ACK：
 * 主机在第 9 个时钟拉低 SDA。
 */
void MyI2C_Ack(void) // 发送ACK应答
{
	MyI2C_W_SCL(0);
	MyI2C_Set_SDA_Output();
	MyI2C_W_SDA(0);
	MyI2C_DelayByBaseUs(2U);
	MyI2C_W_SCL(1);
	MyI2C_DelayByBaseUs(2U);
	MyI2C_W_SCL(0);
}

/*
 * 发送 NACK：
 * 主机在第 9 个时钟保持 SDA 高。
 */
void MyI2C_NAck(void) // 发送NACK应答
{
	MyI2C_W_SCL(0);
	MyI2C_Set_SDA_Output();
	MyI2C_W_SDA(1);
	MyI2C_DelayByBaseUs(2U);
	MyI2C_W_SCL(1);
	MyI2C_DelayByBaseUs(2U);
	MyI2C_W_SCL(0);
}

/*
 * 发送 1 个字节：
 * 逐位输出高位到低位，维持你原先的 5us/2us 时序。
 */
void MyI2C_SendByte(uint8_t Byte) // I2C发送一个字节
{
	MyI2C_Set_SDA_Output();
	for (uint8_t i = 0; i < 8; i++)
	{
		MyI2C_W_SDA((Byte & 0x80) >> 7);
		Byte <<= 1;
		MyI2C_DelayByBaseUs(2U);
		MyI2C_W_SCL(1);
		MyI2C_DelayByBaseUs(2U);
		MyI2C_W_SCL(0);
		MyI2C_DelayByBaseUs(2U);
	}
}

/***********************************************************
*	I2C 写一个字节
*	Ack : 1 发送ACK 0 发送NACK
*	返回    : 接收到的字节
**********************************************************/
/*
 * 接收 1 个字节：
 * Ack = 1 时回 ACK，Ack = 0 时回 NACK。
 * 这段按你原始验证版本保留，不修改协议细节。
 */
uint8_t MyI2C_ReceiveByte(unsigned char Ack) // I2C接收一个字节
{
	unsigned char i, Byte = 0;
	MyI2C_Set_SDA_Input();
	for (i = 0; i < 8; i++)
	{
		MyI2C_W_SCL(0);
		MyI2C_DelayByBaseUs(2U);
		MyI2C_W_SCL(1);
		Byte <<= 1;
		if (MyI2C_R_SDA())
		{
			Byte++;
		}
		MyI2C_DelayByBaseUs(1U);
	}
	if (Ack)
	{
		MyI2C_Ack();
	}
	else
	{
		MyI2C_NAck();
	}
	return Byte;
}

/* 扫描指定总线并输出在线设备地址。 */
static void App_I2C_ScanBus(MyI2C_BusId_t busId)
{
	uint8_t addr;
	uint8_t foundCount;
	const MyI2C_Config_t *config;
	MyI2C_BusId_t prevBusId;
	I2C_SpeedTypeDef prevSpeed;

	config = MyI2C_GetConfigById(busId);
	if (config == 0)
	{
		return;
	}

	prevBusId = s_activeBusId;
	prevSpeed = s_i2cSpeed;

	MyI2C_SelectBus(busId);
	MyI2C_SetSpeed(I2C_SPEED_100K);

	foundCount = 0U;
	usart_printf(USART1, "\r\n[I2C][My_I2C%u] scan start\r\n", (unsigned int)((uint8_t)busId + 1U));

	for (addr = 1U; addr < 0x7FU; addr++)
	{
		MyI2C_Start();
		MyI2C_SendByte((uint8_t)(addr << 1));
		if (MyI2C_Wait_Ack() == 0U)
		{
			foundCount++;
			usart_printf(USART1, "[I2C][My_I2C%u] found: 0x%02X\r\n", (unsigned int)((uint8_t)busId + 1U), addr);
		}
		MyI2C_Stop();
		Delay_ms(1U);
	}

	usart_printf(USART1, "[I2C][My_I2C%u] scan done, count=%u\r\n", (unsigned int)((uint8_t)busId + 1U), foundCount);

	MyI2C_SelectBus(prevBusId);
	MyI2C_SetSpeed(prevSpeed);
}

/*
 * 最小 I2C 测试例程：
 * - 遍历已注册的总线，逐路扫描 7-bit 地址空间。
 * - 发现设备后打印总线号与地址，便于快速确认每路连线。
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
		App_I2C_ScanBus((MyI2C_BusId_t)s_i2cTable[i].id);
	}
}

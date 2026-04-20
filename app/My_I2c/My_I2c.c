#include "My_I2c.h"

#include "Delay.h"
#include "usart.h"
/*
 * 软件 I2C 的板级资源注册表：
 * 目前按单路总线使用，只取第 1 个配置项。
 * 如果后续扩展多路 I2C，可在这里增加按索引选择配置的接口。
 */
static const MyI2C_Config_t *s_i2cTable;
static uint8_t s_i2cCount;

/*
 * 获取当前软件 I2C 使用的板级配置。
 * 返回 0 表示还未调用 MyI2C_Register 注册映射。
 */
static const MyI2C_Config_t *MyI2C_GetConfig(void)
{
	if ((s_i2cTable == 0) || (s_i2cCount == 0U))
	{
		return 0;
	}

	return &s_i2cTable[0];
}

/*
 * 注册板级 I2C 资源表。
 * 由 Enroll_I2C_Register 在系统初始化阶段调用。
 */
void MyI2C_Register(const MyI2C_Config_t *configTable, uint8_t count)
{
	s_i2cTable = configTable;
	s_i2cCount = count;
}

/*
 * 初始化软件 I2C：
 * 1) SCL/SDA 配置为输出
 * 2) 拉高 SCL/SDA，进入 I2C 空闲态
 */
void MyI2C_Init(void)
{
	const MyI2C_Config_t *config;

	config = MyI2C_GetConfig();
	if (config == 0)
	{
		return;
	}

	API_GPIO_InitOutput(config->port, config->sclPin);
	API_GPIO_InitOutput(config->port, config->sdaPin);
	API_GPIO_Write(config->port, config->sclPin, 1U);
	API_GPIO_Write(config->port, config->sdaPin, 1U);
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

	API_GPIO_Write(config->port, config->sclPin, BitValue);
	Delay_us(5);
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

	API_GPIO_Write(config->port, config->sdaPin, BitValue);
	Delay_us(5);
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

	Delay_us(5);
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

	API_GPIO_InitInput(config->port, config->sdaPin);
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
	Delay_us(4);
	MyI2C_W_SDA(0);
	Delay_us(4);
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
    Delay_us(4);
    MyI2C_W_SCL(1);
    MyI2C_W_SDA(1);
    Delay_us(4);
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
	Delay_us(1);
	MyI2C_W_SCL(1);
	Delay_us(1);
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
	Delay_us(2);
	MyI2C_W_SCL(1);
	Delay_us(2);
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
	Delay_us(2);
	MyI2C_W_SCL(1);
	Delay_us(2);
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
		Delay_us(2);
		MyI2C_W_SCL(1);
		Delay_us(2);
		MyI2C_W_SCL(0);
		Delay_us(2);
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
		Delay_us(2);
		MyI2C_W_SCL(1);
		Byte <<= 1;
		if (MyI2C_R_SDA())
		{
			Byte++;
		}
		Delay_us(1);
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

/*
 * 最小 I2C 测试例程：
 * - 扫描 7-bit 地址空间，发送写地址并检测 ACK。
 * - 发现设备后通过 USART1 打印地址，便于上板快速验证线路和时序。
 */
void App_I2C_ScanOnce(void)
{
	uint8_t addr;
	uint8_t foundCount;

	foundCount = 0U;
	usart_printf(USART1, "\r\n[I2C] scan start\r\n");

	for (addr = 1U; addr < 0x7FU; addr++)
	{
		MyI2C_Start();
		MyI2C_SendByte((uint8_t)(addr << 1));
		if (MyI2C_Wait_Ack() == 0U)
		{
			foundCount++;
			usart_printf(USART1, "[I2C] found: 0x%02X\r\n", addr);
		}
		MyI2C_Stop();
		Delay_ms(1U);
	}
	usart_printf(USART1, "[I2C] scan done, count=%u\r\n", foundCount);
}

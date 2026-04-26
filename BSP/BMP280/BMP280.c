#include "BMP280.h"

#include <math.h>

#include "My_I2c.h"

/* 最新一次计算得到的海拔高度（单位：m）。 */
float alt = 0.0f;

/* BMP280 气压/温度过采样与工作模式配置。 */
#define BMP280_PRESSURE_OSR      (BMP280_OVERSAMP_8X)
#define BMP280_TEMPERATURE_OSR   (BMP280_OVERSAMP_16X)
#define BMP280_MODE              ((BMP280_PRESSURE_OSR << 2) | (BMP280_TEMPERATURE_OSR << 5) | BMP280_NORMAL_MODE)

typedef struct
{
	/* 温度补偿参数（来自芯片 NVM 校准区）。 */
	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;
	/* 气压补偿参数（来自芯片 NVM 校准区）。 */
	uint16_t dig_P1;
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;
	/* 中间变量：温度补偿结果，供气压补偿复用。 */
	int32_t t_fine;
} bmp280Calib_t;

/* BMP280 校准参数缓存。 */
static bmp280Calib_t bmp280Cal;

/* 芯片 ID 缓存。 */
static uint8_t bmp280ID = 0U;
/* 初始化标记，避免重复配置。 */
static bool isInit = false;
/* 20-bit 原始气压 ADC 数据。 */
static int32_t bmp280RawPressure = 0;
/* 20-bit 原始温度 ADC 数据。 */
static int32_t bmp280RawTemperature = 0;

/* 读取一帧原始温度/气压数据。 */
static void bmp280GetPressure(void);
/* 气压限幅平均滤波。 */
static void presssureFilter(float *in, float *out);
/* 气压值转换为海拔高度。 */
static float bmp280PressureToAltitude(float *pressure);
/* 温度补偿（输出单位：0.01 摄氏度）。 */
static int32_t bmp280CompensateT(int32_t adcT);
/* 气压补偿（输出单位：Q24.8 Pa）。 */
static uint32_t bmp280CompensateP(int32_t adcP);


/*  选择I2C1 设置BMP280 I2C速率为400kHZ */
static void BMP280_SelectI2CSpeed(void)
{
	MyI2C_SelectBus(BMP280_I2C_BUS);
	MyI2C_SetSpeed(BMP280_I2C_SPEED);
}

/*
 * 读取单个寄存器。
 * devaddr: 8-bit I2C 地址（写地址）
 * addr   : 目标寄存器地址
 * return : 读取到的 1 字节数据
 */
uint8_t iicDevReadByte(uint8_t devaddr, uint8_t addr)
{
	uint8_t temp;

	BMP280_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte(devaddr);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(addr);
	MyI2C_Wait_Ack();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)(devaddr | 0x01U));
	MyI2C_Wait_Ack();
	temp = MyI2C_ReceiveByte(0U);
	MyI2C_Stop();

	return temp;
}

/*
 * 连续读取多个寄存器。
 * devaddr: 8-bit I2C 地址（写地址）
 * addr   : 起始寄存器地址
 * len    : 读取长度
 * rbuf   : 输出缓存
 */
void iicDevRead(uint8_t devaddr, uint8_t addr, uint8_t len, uint8_t *rbuf)
{
	uint8_t i;

	BMP280_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte(devaddr);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(addr);
	MyI2C_Wait_Ack();

	MyI2C_Start();
	MyI2C_SendByte((uint8_t)(devaddr | 0x01U));
	MyI2C_Wait_Ack();

	for (i = 0U; i < len; i++)
	{
		if (i == (uint8_t)(len - 1U))
		{
			rbuf[i] = MyI2C_ReceiveByte(0U);
		}
		else
		{
			rbuf[i] = MyI2C_ReceiveByte(1U);
		}
	}

	MyI2C_Stop();
}

/*
 * 写入单个寄存器。
 * devaddr: 8-bit I2C 地址（写地址）
 * addr   : 目标寄存器地址
 * data   : 写入数据
 */
void iicDevWriteByte(uint8_t devaddr, uint8_t addr, uint8_t data)
{
	BMP280_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte(devaddr);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(addr);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(data);
	MyI2C_Wait_Ack();
	MyI2C_Stop();
}

/*
 * 连续写入多个寄存器。
 * devaddr: 8-bit I2C 地址（写地址）
 * addr   : 起始寄存器地址
 * len    : 写入长度
 * wbuf   : 输入缓存
 */
void iicDevWrite(uint8_t devaddr, uint8_t addr, uint8_t len, uint8_t *wbuf)
{
	uint8_t i;

	BMP280_SelectI2CSpeed();

	MyI2C_Start();
	MyI2C_SendByte(devaddr);
	MyI2C_Wait_Ack();
	MyI2C_SendByte(addr);
	MyI2C_Wait_Ack();

	for (i = 0U; i < len; i++)
	{
		MyI2C_SendByte(wbuf[i]);
		MyI2C_Wait_Ack();
	}

	MyI2C_Stop();
}

/*
 * BMP280 初始化流程：
 * 1) 读取并校验芯片 ID
 * 2) 读取 24 字节校准参数
 * 3) 配置温度/气压过采样和工作模式
 * 4) 配置内部 IIR 滤波
 */
bool BMP280Init(void)
{
	if (isInit)
	{
		return true;
	}

	bmp280ID = iicDevReadByte(BMP280_ADDR, BMP280_CHIP_ID);
	if (bmp280ID != BMP280_DEFAULT_CHIP_ID)
	{
		return false;
	}

	iicDevRead(BMP280_ADDR,
			   BMP280_TEMPERATURE_CALIB_DIG_T1_LSB_REG,
			   BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH,
			   (uint8_t *)&bmp280Cal);

	iicDevWriteByte(BMP280_ADDR, BMP280_CTRL_MEAS_REG, BMP280_MODE);
	iicDevWriteByte(BMP280_ADDR, BMP280_CONFIG_REG, (uint8_t)((0U << 5) | (4U << 2) | 0U));

	isInit = true;
	return true;
}

/* 从数据寄存器读取一帧 6 字节原始数据并拆包为 20-bit ADC 值。 */
static void bmp280GetPressure(void)
{
	uint8_t data[BMP280_DATA_FRAME_SIZE];

	iicDevRead(BMP280_ADDR, BMP280_PRESSURE_MSB_REG, BMP280_DATA_FRAME_SIZE, data);
	bmp280RawPressure = (int32_t)((((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4)));
	bmp280RawTemperature = (int32_t)((((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)data[5] >> 4)));
}

/*
 * 温度补偿（Bosch 推荐公式）。
 * adcT: 原始温度 ADC
 * return: 温度值，单位 0.01 摄氏度
 */
static int32_t bmp280CompensateT(int32_t adcT)
{
	/* 温补公式中间变量。 */
	int32_t var1;
	int32_t var2;
	/* 补偿后的温度输出（0.01 摄氏度）。 */
	int32_t t;

	var1 = ((((adcT >> 3) - ((int32_t)bmp280Cal.dig_T1 << 1)))*((int32_t)bmp280Cal.dig_T2)) >> 11;
	var2 = (((((adcT >> 4) - ((int32_t)bmp280Cal.dig_T1)) * ((adcT >> 4) - ((int32_t)bmp280Cal.dig_T1))) >> 12) * ((int32_t)bmp280Cal.dig_T3)) >> 14;
	bmp280Cal.t_fine = var1 + var2;

	t = (bmp280Cal.t_fine * 5 + 128) >> 8;
	return t;
}

/*
 * 气压补偿（Bosch 推荐公式，64-bit 防溢出）。
 * adcP: 原始气压 ADC
 * return: Q24.8 格式的 Pa
 */
static uint32_t bmp280CompensateP(int32_t adcP)
{
	/* 气压补偿公式中间变量。 */
	int64_t var1;
	int64_t var2;
	/* 补偿结果（Q24.8）。 */
	int64_t p;

	var1 = ((int64_t)bmp280Cal.t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)bmp280Cal.dig_P6;
	var2 = var2 + ((var1 * (int64_t)bmp280Cal.dig_P5) << 17);
	var2 = var2 + (((int64_t)bmp280Cal.dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)bmp280Cal.dig_P3) >> 8) + ((var1 * (int64_t)bmp280Cal.dig_P2) << 12);
	var1 = (((((int64_t)1) << 47) + var1) * ((int64_t)bmp280Cal.dig_P1)) >> 33;

	if (var1 == 0)
	{
		return 0U;
	}

	p = 1048576 - adcP;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)bmp280Cal.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)bmp280Cal.dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280Cal.dig_P7) << 4);

	return (uint32_t)p;
}

#define FILTER_NUM  5U
#define FILTER_A    0.1f

/*
 * 限幅平均滤波：
 * - FILTER_A: 限幅阈值
 * - FILTER_NUM: 均值窗口长度
 */
static void presssureFilter(float *in, float *out)
{
	/* 环形缓冲写入索引。 */
	static uint8_t i = 0U;
	/* 气压滤波缓冲区。 */
	static float filter_buf[FILTER_NUM] = {0.0f};
	/* 累加求平均。 */
	float filter_sum = 0.0f;
	/* 循环计数。 */
	uint8_t cnt;
	/* 当前输入与上一次有效值的差值。 */
	float deta;

	if (filter_buf[i] == 0.0f)
	{
		filter_buf[i] = *in;
		*out = *in;
		i++;
		if (i >= FILTER_NUM)
		{
			i = 0U;
		}
	}
	else
	{
		if (i != 0U)
		{
			deta = *in - filter_buf[i - 1U];
		}
		else
		{
			deta = *in - filter_buf[FILTER_NUM - 1U];
		}

		if (fabsf(deta) < FILTER_A)
		{
			filter_buf[i] = *in;
			i++;
			if (i >= FILTER_NUM)
			{
				i = 0U;
			}
		}

		for (cnt = 0U; cnt < FILTER_NUM; cnt++)
		{
			filter_sum += filter_buf[cnt];
		}
		*out = filter_sum / (float)FILTER_NUM;
	}
}

/*
 * 获取 BMP280 实测数据：
 * pressure   -> 气压（hPa）
 * temperature-> 温度（摄氏度）
 * asl        -> 海拔（m）
 */
void BMP280GetData(float *pressure, float *temperature, float *asl)
{
	/* 温度中间量。 */
	float t;
	/* 气压中间量。 */
	float p;

	bmp280GetPressure();

	t = (float)bmp280CompensateT(bmp280RawTemperature) / 100.0f;
	p = (float)bmp280CompensateP(bmp280RawPressure) / 25600.0f;

	presssureFilter(&p, pressure);
	*temperature = t;
	*pressure = p;
	*asl = bmp280PressureToAltitude(pressure);
	alt = *asl;
}

/* 标准大气压换算指数 1/5.25588。 */
#define CONST_PF  0.1902630958f
/* 固定温度（摄氏度），避免环境温漂导致海拔抖动。 */
#define FIX_TEMP  25.0f

/* 根据当前气压估算海拔高度（单位：m）。 */
static float bmp280PressureToAltitude(float *pressure)
{
	if (*pressure > 0.0f)
	{
		return ((powf((1015.7f / *pressure), CONST_PF) - 1.0f) * (FIX_TEMP + 273.15f)) / 0.0065f;
	}

	return 0.0f;
}

/* 仅返回海拔的便捷接口。 */
float BMP_Data(void)
{
	/* 当前气压（hPa）。 */
	float bmp280_press = 0.0f;
	/* 当前温度（摄氏度）。 */
	float bmp280_temp = 0.0f;
	/* 当前海拔（m）。 */
	float bmp280_asl = 0.0f;

	BMP280GetData(&bmp280_press, &bmp280_temp, &bmp280_asl);
	return bmp280_asl;
}

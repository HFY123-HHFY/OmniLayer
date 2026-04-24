#include "OLED.h"

#include "My_I2c.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

/**
	* 数据存储格式：
	* 纵向8点，高位在下，先从左到右，再从上到下；每个Bit对应一个像素点。
	* 坐标定义：左上角为(0,0)，X向右(0~127)，Y向下(0~63)。
	*/

/**
  * 数据存储格式：
  * 纵向8点，高位在下，先从左到右，再从上到下
  * 每一个Bit对应一个像素点
  * 
  *      B0 B0                  B0 B0
  *      B1 B1                  B1 B1
  *      B2 B2                  B2 B2
  *      B3 B3  ------------->  B3 B3 --
  *      B4 B4                  B4 B4  |
  *      B5 B5                  B5 B5  |
  *      B6 B6                  B6 B6  |
  *      B7 B7                  B7 B7  |
  *                                    |
  *  -----------------------------------
  *  |   
  *  |   B0 B0                  B0 B0
  *  |   B1 B1                  B1 B1
  *  |   B2 B2                  B2 B2
  *  --> B3 B3  ------------->  B3 B3
  *      B4 B4                  B4 B4
  *      B5 B5                  B5 B5
  *      B6 B6                  B6 B6
  *      B7 B7                  B7 B7
  * 
  * 坐标轴定义：
  * 左上角为(0, 0)点
  * 横向向右为X轴，取值范围：0~127
  * 纵向向下为Y轴，取值范围：0~63
  * 
  *       0             X轴           127 
  *      .------------------------------->
  *    0 |
  *      |
  *      |
  *      |
  *  Y轴 |
  *      |
  *      |
  *      |
  *   63 |
  *      v
  * 
  */

/*全局变量*********************/
/** OLED显存数组（8页x128列） */
uint8_t OLED_DisplayBuf[8][128];
/* ********************全局变量*/

/*  选择I2C2 设置OLED I2C速率为400kHZ */
static void OLED_SelectI2CSpeed(void)
{
	MyI2C_SelectBus(My_I2C2);
	MyI2C_SetSpeed(I2C_SPEED_400K);
}

/**
	* 函    数：OLED引脚初始化
	* 说    明：按当前仓库GPIO抽象初始化输出并释放总线
	*/
void OLED_GPIO_Init(void)
{
	uint32_t i = 0, j = 0;

	OLED_SelectI2CSpeed();

    /*在初始化前，加入适量延时，待OLED供电稳定*/
	for (i = 0U; i < 1000U; i++)
	{
		for (j = 0U; j < 1000U; j++);
	}

    /*释放SCL和SDA*/
	MyI2C_W_SCL(1U);
	MyI2C_W_SDA(1U);
}


/** 函    数：OLED I2C发送一个字节 */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;

	for (i = 0U; i < 8U; i++)
	{
		MyI2C_W_SDA((uint8_t)!!(Byte & (uint8_t)(0x80U >> i)));
		MyI2C_W_SCL(1U);
		MyI2C_W_SCL(0U);
	}

	MyI2C_W_SCL(1U);
	MyI2C_W_SCL(0U);
}

/** 函    数：OLED写命令 */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_SelectI2CSpeed();
	MyI2C_Start();
	OLED_I2C_SendByte(0x78U);
	OLED_I2C_SendByte(0x00U);
	OLED_I2C_SendByte(Command);
	MyI2C_Stop();
}

/** 函    数：OLED写数据 */
void OLED_WriteData(uint8_t *Data, uint8_t Count)
{
	uint8_t i;

	OLED_SelectI2CSpeed();

	MyI2C_Start();
	OLED_I2C_SendByte(0x78U);
	OLED_I2C_SendByte(0x40U);
	for (i = 0U; i < Count; i++)
	{
		OLED_I2C_SendByte(Data[i]);
	}
	MyI2C_Stop();
}

/** 函    数：设置页地址和列地址 */
void OLED_SetCursor(uint8_t Page, uint8_t X)
{
	OLED_WriteCommand((uint8_t)(0xB0U | Page));
	OLED_WriteCommand((uint8_t)(0x10U | ((X & 0xF0U) >> 4)));
	OLED_WriteCommand((uint8_t)(0x00U | (X & 0x0FU)));
}

/** 工具函数：整数次幂 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1U;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/** 工具函数：点在多边形内判断 */
uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
	int16_t i;
	int16_t j;
	int16_t c = 0;

	for (i = 0, j = (int16_t)(nvert - 1U); i < nvert; j = i++)
	{
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
		{
			c = (int16_t)!c;
		}
	}
	return (uint8_t)c;
}

/** 工具函数：点在角度范围内判断 */
uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
	int16_t PointAngle;

	PointAngle = (int16_t)(atan2((double)Y, (double)X) / 3.14 * 180.0);
	if (StartAngle < EndAngle)
	{
		if (PointAngle >= StartAngle && PointAngle <= EndAngle)
		{
			return 1U;
		}
	}
	else
	{
		if (PointAngle >= StartAngle || PointAngle <= EndAngle)
		{
			return 1U;
		}
	}
	return 0U;
}

/**
  * 函    数：OLED初始化
  * 说    明：按SSD1306常用初始化序列配置
  */
void OLED_Init(void)
{
	OLED_GPIO_Init();

	OLED_WriteCommand(0xAE);
	OLED_WriteCommand(0xD5);
	OLED_WriteCommand(0x80);
	OLED_WriteCommand(0xA8);
	OLED_WriteCommand(0x3F);
	OLED_WriteCommand(0xD3);
	OLED_WriteCommand(0x00);
	OLED_WriteCommand(0x40);
	OLED_WriteCommand(0xA1);
	OLED_WriteCommand(0xC8);
	OLED_WriteCommand(0xDA);
	OLED_WriteCommand(0x12);
	OLED_WriteCommand(0x81);
	OLED_WriteCommand(0xCF);
	OLED_WriteCommand(0xD9);
	OLED_WriteCommand(0xF1);
	OLED_WriteCommand(0xDB);
	OLED_WriteCommand(0x30);
	OLED_WriteCommand(0xA4);
	OLED_WriteCommand(0xA6);
	OLED_WriteCommand(0x8D);
	OLED_WriteCommand(0x14);
	OLED_WriteCommand(0xAF);

	OLED_Clear();
	OLED_Update();
}

/** 函    数：全屏刷新 */
void OLED_Update(void)
{
	uint8_t j;
	for (j = 0U; j < 8U; j++)
	{
		OLED_SetCursor(j, 0U);
		OLED_WriteData(OLED_DisplayBuf[j], 128U);
	}
}

/** 函    数：区域刷新 */
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t j;
	int16_t Page;
	int16_t Page1;

	Page = Y / 8;
	Page1 = (Y + Height - 1) / 8 + 1;
	if (Y < 0)
	{
		Page -= 1;
		Page1 -= 1;
	}

	for (j = Page; j < Page1; j++)
	{
		if (X >= 0 && X <= 127 && j >= 0 && j <= 7)
		{
			OLED_SetCursor((uint8_t)j, (uint8_t)X);
			OLED_WriteData(&OLED_DisplayBuf[j][X], Width);
		}
	}
}

/** 函    数：清空显存 */
void OLED_Clear(void)
{
	uint8_t i;
	uint8_t j;
	for (j = 0U; j < 8U; j++)
	{
		for (i = 0U; i < 128U; i++)
		{
			OLED_DisplayBuf[j][i] = 0x00U;
		}
	}
}

/** 函    数：区域清空 */
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i;
	int16_t j;

	for (j = Y; j < Y + Height; j++)
	{
		for (i = X; i < X + Width; i++)
		{
			if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
			{
				OLED_DisplayBuf[j / 8][i] &= (uint8_t)~(0x01U << (j % 8));
			}
		}
	}
}

/** 函    数：全屏取反 */
void OLED_Reverse(void)
{
	uint8_t i;
	uint8_t j;
	for (j = 0U; j < 8U; j++)
	{
		for (i = 0U; i < 128U; i++)
		{
			OLED_DisplayBuf[j][i] ^= 0xFFU;
		}
	}
}

/** 函    数：区域取反 */
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i;
	int16_t j;

	for (j = Y; j < Y + Height; j++)
	{
		for (i = X; i < X + Width; i++)
		{
			if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
			{
				OLED_DisplayBuf[j / 8][i] ^= (uint8_t)(0x01U << (j % 8));
			}
		}
	}
}

/** 函    数：显示单字符 */
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
	if (FontSize == OLED_8X16)
	{
		OLED_ShowImage(X, Y, 8U, 16U, OLED_F8x16[(uint8_t)(Char - ' ')]);
	}
	else if (FontSize == OLED_6X8)
	{
		OLED_ShowImage(X, Y, 6U, 8U, OLED_F6x8[(uint8_t)(Char - ' ')]);
	}
}

/** 函    数：显示字符串（ASCII/中文混合） */
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
	uint16_t i = 0U;
	char SingleChar[5];
	uint8_t CharLength = 0U;
	uint16_t XOffset = 0U;
	uint16_t pIndex;

	while (String[i] != '\0')
	{
#ifdef OLED_CHARSET_UTF8
		if ((String[i] & 0x80) == 0x00)
		{
			CharLength = 1U;
			SingleChar[0] = String[i++];
			SingleChar[1] = '\0';
		}
		else if ((String[i] & 0xE0) == 0xC0)
		{
			CharLength = 2U;
			SingleChar[0] = String[i++];
			if (String[i] == '\0') { break; }
			SingleChar[1] = String[i++];
			SingleChar[2] = '\0';
		}
		else if ((String[i] & 0xF0) == 0xE0)
		{
			CharLength = 3U;
			SingleChar[0] = String[i++];
			if (String[i] == '\0') { break; }
			SingleChar[1] = String[i++];
			if (String[i] == '\0') { break; }
			SingleChar[2] = String[i++];
			SingleChar[3] = '\0';
		}
		else if ((String[i] & 0xF8) == 0xF0)
		{
			CharLength = 4U;
			SingleChar[0] = String[i++];
			if (String[i] == '\0') { break; }
			SingleChar[1] = String[i++];
			if (String[i] == '\0') { break; }
			SingleChar[2] = String[i++];
			if (String[i] == '\0') { break; }
			SingleChar[3] = String[i++];
			SingleChar[4] = '\0';
		}
		else
		{
			i++;
			continue;
		}
#endif

#ifdef OLED_CHARSET_GB2312
		if ((String[i] & 0x80) == 0x00)
		{
			CharLength = 1U;
			SingleChar[0] = String[i++];
			SingleChar[1] = '\0';
		}
		else
		{
			CharLength = 2U;
			SingleChar[0] = String[i++];
			if (String[i] == '\0') { break; }
			SingleChar[1] = String[i++];
			SingleChar[2] = '\0';
		}
#endif

		if (CharLength == 1U)
		{
			OLED_ShowChar((int16_t)(X + XOffset), Y, SingleChar[0], FontSize);
			XOffset += FontSize;
		}
		else
		{
			for (pIndex = 0U; strcmp(OLED_CF16x16[pIndex].Index, "") != 0; pIndex++)
			{
				if (strcmp(OLED_CF16x16[pIndex].Index, SingleChar) == 0)
				{
					break;
				}
			}
			if (FontSize == OLED_8X16)
			{
				OLED_ShowImage((int16_t)(X + XOffset), Y, 16U, 16U, OLED_CF16x16[pIndex].Data);
				XOffset += 16U;
			}
			else if (FontSize == OLED_6X8)
			{
				OLED_ShowChar((int16_t)(X + XOffset), Y, '?', OLED_6X8);
				XOffset += OLED_6X8;
			}
		}
	}
}

/** 函    数：显示无符号十进制 */
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0U; i < Length; i++)
	{
		OLED_ShowChar((int16_t)(X + i * FontSize), Y, (char)(Number / OLED_Pow(10U, Length - i - 1U) % 10U + '0'), FontSize);
	}
}

/** 函    数：显示有符号十进制 */
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	uint32_t Number1;

	if (Number >= 0)
	{
		OLED_ShowChar(X, Y, '+', FontSize);
		Number1 = (uint32_t)Number;
	}
	else
	{
		OLED_ShowChar(X, Y, '-', FontSize);
		Number1 = (uint32_t)(-Number);
	}

	for (i = 0U; i < Length; i++)
	{
		OLED_ShowChar((int16_t)(X + (i + 1U) * FontSize), Y, (char)(Number1 / OLED_Pow(10U, Length - i - 1U) % 10U + '0'), FontSize);
	}
}

/** 函    数：显示十六进制 */
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	uint8_t SingleNumber;
	for (i = 0U; i < Length; i++)
	{
		SingleNumber = (uint8_t)(Number / OLED_Pow(16U, Length - i - 1U) % 16U);
		if (SingleNumber < 10U)
		{
			OLED_ShowChar((int16_t)(X + i * FontSize), Y, (char)(SingleNumber + '0'), FontSize);
		}
		else
		{
			OLED_ShowChar((int16_t)(X + i * FontSize), Y, (char)(SingleNumber - 10U + 'A'), FontSize);
		}
	}
}

/** 函    数：显示二进制 */
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0U; i < Length; i++)
	{
		OLED_ShowChar((int16_t)(X + i * FontSize), Y, (char)(Number / OLED_Pow(2U, Length - i - 1U) % 2U + '0'), FontSize);
	}
}

/** 函    数：显示浮点数（四舍五入） */
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
	uint32_t PowNum;
	uint32_t IntNum;
	uint32_t FraNum;

	if (Number >= 0)
	{
		OLED_ShowChar(X, Y, '+', FontSize);
	}
	else
	{
		OLED_ShowChar(X, Y, '-', FontSize);
		Number = -Number;
	}

	IntNum = (uint32_t)Number;
	Number -= IntNum;
	PowNum = OLED_Pow(10U, FraLength);
	FraNum = (uint32_t)round(Number * PowNum);
	IntNum += FraNum / PowNum;

	OLED_ShowNum((int16_t)(X + FontSize), Y, IntNum, IntLength, FontSize);
	OLED_ShowChar((int16_t)(X + (IntLength + 1U) * FontSize), Y, '.', FontSize);
	OLED_ShowNum((int16_t)(X + (IntLength + 2U) * FontSize), Y, FraNum, FraLength, FontSize);
}

/** 函    数：显示位图 */
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
	uint8_t i = 0U;
	uint8_t j = 0U;
	int16_t Page;
	int16_t Shift;

	OLED_ClearArea(X, Y, Width, Height);

	for (j = 0U; j < (Height - 1U) / 8U + 1U; j++)
	{
		for (i = 0U; i < Width; i++)
		{
			if (X + i >= 0 && X + i <= 127)
			{
				Page = Y / 8;
				Shift = Y % 8;
				if (Y < 0)
				{
					Page -= 1;
					Shift += 8;
				}

				if (Page + j >= 0 && Page + j <= 7)
				{
					OLED_DisplayBuf[Page + j][X + i] |= (uint8_t)(Image[j * Width + i] << Shift);
				}

				if (Page + j + 1 >= 0 && Page + j + 1 <= 7)
				{
					OLED_DisplayBuf[Page + j + 1][X + i] |= (uint8_t)(Image[j * Width + i] >> (8 - Shift));
				}
			}
		}
	}
}

/** 函    数：格式化显示字符串 */
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
	char String[256];
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);
	OLED_ShowString(X, Y, String, FontSize);
}

/** 函    数：画点 */
void OLED_DrawPoint(int16_t X, int16_t Y)
{
	if (X >= 0 && X <= 127 && Y >= 0 && Y <= 63)
	{
		OLED_DisplayBuf[Y / 8][X] |= (uint8_t)(0x01U << (Y % 8));
	}
}

/** 函    数：读点 */
uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
	if (X >= 0 && X <= 127 && Y >= 0 && Y <= 63)
	{
		if (OLED_DisplayBuf[Y / 8][X] & (uint8_t)(0x01U << (Y % 8)))
		{
			return 1U;
		}
	}

	return 0U;
}

/** 函    数：画线 */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
	int16_t x;
	int16_t y;
	int16_t dx;
	int16_t dy;
	int16_t d;
	int16_t incrE;
	int16_t incrNE;
	int16_t temp;
	int16_t x0 = X0;
	int16_t y0 = Y0;
	int16_t x1 = X1;
	int16_t y1 = Y1;
	uint8_t yflag = 0U;
	uint8_t xyflag = 0U;

	if (y0 == y1)
	{
		if (x0 > x1) { temp = x0; x0 = x1; x1 = temp; }
		for (x = x0; x <= x1; x++)
		{
			OLED_DrawPoint(x, y0);
		}
	}
	else if (x0 == x1)
	{
		if (y0 > y1) { temp = y0; y0 = y1; y1 = temp; }
		for (y = y0; y <= y1; y++)
		{
			OLED_DrawPoint(x0, y);
		}
	}
	else
	{
		if (x0 > x1)
		{
			temp = x0; x0 = x1; x1 = temp;
			temp = y0; y0 = y1; y1 = temp;
		}

		if (y0 > y1)
		{
			y0 = -y0;
			y1 = -y1;
			yflag = 1U;
		}

		if (y1 - y0 > x1 - x0)
		{
			temp = x0; x0 = y0; y0 = temp;
			temp = x1; x1 = y1; y1 = temp;
			xyflag = 1U;
		}

		dx = x1 - x0;
		dy = y1 - y0;
		incrE = 2 * dy;
		incrNE = 2 * (dy - dx);
		d = 2 * dy - dx;
		x = x0;
		y = y0;

		if (yflag && xyflag) { OLED_DrawPoint(y, -x); }
		else if (yflag)      { OLED_DrawPoint(x, -y); }
		else if (xyflag)     { OLED_DrawPoint(y, x); }
		else                 { OLED_DrawPoint(x, y); }

		while (x < x1)
		{
			x++;
			if (d < 0)
			{
				d += incrE;
			}
			else
			{
				y++;
				d += incrNE;
			}

			if (yflag && xyflag) { OLED_DrawPoint(y, -x); }
			else if (yflag)      { OLED_DrawPoint(x, -y); }
			else if (xyflag)     { OLED_DrawPoint(y, x); }
			else                 { OLED_DrawPoint(x, y); }
		}
	}
}

/** 函    数：画矩形 */
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
	int16_t i;
	int16_t j;
	if (!IsFilled)
	{
		for (i = X; i < X + Width; i++)
		{
			OLED_DrawPoint(i, Y);
			OLED_DrawPoint(i, Y + Height - 1);
		}
		for (i = Y; i < Y + Height; i++)
		{
			OLED_DrawPoint(X, i);
			OLED_DrawPoint(X + Width - 1, i);
		}
	}
	else
	{
		for (i = X; i < X + Width; i++)
		{
			for (j = Y; j < Y + Height; j++)
			{
				OLED_DrawPoint(i, j);
			}
		}
	}
}

/** 函    数：画三角形 */
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
	int16_t minx = X0;
	int16_t miny = Y0;
	int16_t maxx = X0;
	int16_t maxy = Y0;
	int16_t i;
	int16_t j;
	int16_t vx[] = {X0, X1, X2};
	int16_t vy[] = {Y0, Y1, Y2};

	if (!IsFilled)
	{
		OLED_DrawLine(X0, Y0, X1, Y1);
		OLED_DrawLine(X0, Y0, X2, Y2);
		OLED_DrawLine(X1, Y1, X2, Y2);
	}
	else
	{
		if (X1 < minx) { minx = X1; }
		if (X2 < minx) { minx = X2; }
		if (Y1 < miny) { miny = Y1; }
		if (Y2 < miny) { miny = Y2; }

		if (X1 > maxx) { maxx = X1; }
		if (X2 > maxx) { maxx = X2; }
		if (Y1 > maxy) { maxy = Y1; }
		if (Y2 > maxy) { maxy = Y2; }

		for (i = minx; i <= maxx; i++)
		{
			for (j = miny; j <= maxy; j++)
			{
				if (OLED_pnpoly(3U, vx, vy, i, j)) { OLED_DrawPoint(i, j); }
			}
		}
	}
}

/** 函    数：画圆 */
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
	int16_t x;
	int16_t y;
	int16_t d;
	int16_t j;

	d = 1 - Radius;
	x = 0;
	y = Radius;

	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X + y, Y + x);
	OLED_DrawPoint(X - y, Y - x);

	if (IsFilled)
	{
		for (j = -y; j < y; j++)
		{
			OLED_DrawPoint(X, Y + j);
		}
	}

	while (x < y)
	{
		x++;
		if (d < 0)
		{
			d += 2 * x + 1;
		}
		else
		{
			y--;
			d += 2 * (x - y) + 1;
		}

		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X + y, Y + x);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - y, Y - x);
		OLED_DrawPoint(X + x, Y - y);
		OLED_DrawPoint(X + y, Y - x);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X - y, Y + x);

		if (IsFilled)
		{
			for (j = -y; j < y; j++)
			{
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}

			for (j = -x; j < x; j++)
			{
				OLED_DrawPoint(X - y, Y + j);
				OLED_DrawPoint(X + y, Y + j);
			}
		}
	}
}

/** 函    数：画椭圆 */
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
	int16_t x;
	int16_t y;
	int16_t j;
	int16_t a = A;
	int16_t b = B;
	float d1;
	float d2;

	x = 0;
	y = b;
	d1 = b * b + a * a * (-b + 0.5f);

	if (IsFilled)
	{
		for (j = -y; j < y; j++)
		{
			OLED_DrawPoint(X, Y + j);
			OLED_DrawPoint(X, Y + j);
		}
	}

	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X - x, Y + y);
	OLED_DrawPoint(X + x, Y - y);

	while (b * b * (x + 1) < a * a * (y - 0.5f))
	{
		if (d1 <= 0)
		{
			d1 += (float)(b * b * (2 * x + 3));
		}
		else
		{
			d1 += (float)(b * b * (2 * x + 3) + a * a * (-2 * y + 2));
			y--;
		}
		x++;

		if (IsFilled)
		{
			for (j = -y; j < y; j++)
			{
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}

		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}

	d2 = (float)(b * b) * (x + 0.5f) * (x + 0.5f) + (float)(a * a) * (y - 1) * (y - 1) - (float)(a * a * b * b);

	while (y > 0)
	{
		if (d2 <= 0)
		{
			d2 += (float)(b * b * (2 * x + 2) + a * a * (-2 * y + 3));
			x++;
		}
		else
		{
			d2 += (float)(a * a * (-2 * y + 3));
		}
		y--;

		if (IsFilled)
		{
			for (j = -y; j < y; j++)
			{
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}

		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}
}

/** 函    数：画圆弧/扇形 */
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
	int16_t x;
	int16_t y;
	int16_t d;
	int16_t j;

	d = 1 - Radius;
	x = 0;
	y = Radius;

	if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) { OLED_DrawPoint(X + x, Y + y); }
	if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) { OLED_DrawPoint(X - x, Y - y); }
	if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) { OLED_DrawPoint(X + y, Y + x); }
	if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) { OLED_DrawPoint(X - y, Y - x); }

	if (IsFilled)
	{
		for (j = -y; j < y; j++)
		{
			if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) { OLED_DrawPoint(X, Y + j); }
		}
	}

	while (x < y)
	{
		x++;
		if (d < 0)
		{
			d += 2 * x + 1;
		}
		else
		{
			y--;
			d += 2 * (x - y) + 1;
		}

		if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) { OLED_DrawPoint(X + x, Y + y); }
		if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) { OLED_DrawPoint(X + y, Y + x); }
		if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) { OLED_DrawPoint(X - x, Y - y); }
		if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) { OLED_DrawPoint(X - y, Y - x); }
		if (OLED_IsInAngle(x, -y, StartAngle, EndAngle)) { OLED_DrawPoint(X + x, Y - y); }
		if (OLED_IsInAngle(y, -x, StartAngle, EndAngle)) { OLED_DrawPoint(X + y, Y - x); }
		if (OLED_IsInAngle(-x, y, StartAngle, EndAngle)) { OLED_DrawPoint(X - x, Y + y); }
		if (OLED_IsInAngle(-y, x, StartAngle, EndAngle)) { OLED_DrawPoint(X - y, Y + x); }

		if (IsFilled)
		{
			for (j = -y; j < y; j++)
			{
				if (OLED_IsInAngle(x, j, StartAngle, EndAngle)) { OLED_DrawPoint(X + x, Y + j); }
				if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) { OLED_DrawPoint(X - x, Y + j); }
			}

			for (j = -x; j < x; j++)
			{
				if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) { OLED_DrawPoint(X - y, Y + j); }
				if (OLED_IsInAngle(y, j, StartAngle, EndAngle)) { OLED_DrawPoint(X + y, Y + j); }
			}
		}
	}
}



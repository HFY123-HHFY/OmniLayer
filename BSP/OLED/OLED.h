#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include "OLED_Data.h"

/*参数宏定义*********************/
/* FontSize参数取值（同时作为字符横向偏移宽度） */
#define OLED_8X16               8
#define OLED_6X8                6

/* IsFilled参数取值 */
#define OLED_UNFILLED           0
#define OLED_FILLED             1
/* ********************参数宏定义*/

/* OLED 接口类型：支持 4 针 I2C 与 7 针 SPI。 */
typedef enum
{
	OLED_IF_I2C = 0,
	OLED_IF_SPI
} OLED_Interface_t;

/* OLED 在 SPI 模式下额外使用的控制引脚（不放入通用 SPI 抽象层）。 */
typedef struct
{
	void *dcPort;
	uint16_t dcPin;
	void *resPort;
	uint16_t resPin;
} OLED_SpiCtrlConfig_t;

#ifdef __cplusplus
extern "C" {
#endif

/*函数声明*********************/
/* 注册 OLED SPI 模式下的 DC/RES 板级映射。 */
void OLED_RegisterSpiCtrl(const OLED_SpiCtrlConfig_t *configTable, uint8_t count);

/* 初始化函数：按接口类型选择 I2C 或 SPI 驱动路径。 */
void OLED_Init(OLED_Interface_t interfaceType);

/* 更新函数 */
void OLED_Update(void);
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/* 显存控制函数 */
void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/* 显示函数 */
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

/* 绘图函数 */
void OLED_DrawPoint(int16_t X, int16_t Y);
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);
/* ********************函数声明*/

#ifdef __cplusplus
}
#endif

#endif

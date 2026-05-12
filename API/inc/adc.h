#ifndef __API_ADC_H
#define __API_ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_adc.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_adc.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_G3507)
#include "G3507_adc.h"
#else
#error "Unsupported ENROLL_MCU_TARGET."
#endif

typedef enum
{
	API_ADC1 = 0U,
	API_ADC2 = 1U,
	API_ADC3 = 2U
} API_ADC_Id_t;

/* ADC 通道编号常量。 */
typedef enum
{
	API_ADC_CH0 = 0U,
	API_ADC_CH1 = 1U,
	API_ADC_CH2 = 2U,
	API_ADC_CH3 = 3U,
	API_ADC_CH4 = 4U,
	API_ADC_CH5 = 5U,
	API_ADC_CH6 = 6U,
	API_ADC_CH7 = 7U,
	API_ADC_CH8 = 8U,
	API_ADC_CH9 = 9U,
	API_ADC_CH10 = 10U,
	API_ADC_CH11 = 11U,
	API_ADC_CH12 = 12U,
	API_ADC_CH13 = 13U,
	API_ADC_CH14 = 14U,
	API_ADC_CH15 = 15U,
	API_ADC_CH16 = 16U,
	API_ADC_CH17 = 17U,
	API_ADC_CH18 = 18U
} API_ADC_Channel_t;

typedef struct
{
	/* 选择 ADC 实例。 */
	API_ADC_Id_t adcId;
	/* 选择规则通道号。 */
	API_ADC_Channel_t channel;
	/* 映射到的 GPIO 端口。 */
	void *port;
	/* 映射到的 GPIO 引脚（位掩码）。 */
	uint32_t pin;
} API_ADC_Config_t;

/* 注册板级 ADC 映射表。 */
void API_ADC_Register(const API_ADC_Config_t *configTable, uint8_t count);
/* ADC 初始化函数：x 为要初始化的 ADC 几。 */
void API_ADC_Init(API_ADC_Id_t x);
/* ADC 获取函数：x 选择 ADC 几，y 选择该 ADC 的通道号。 */
uint16_t API_ADC_GetValue(API_ADC_Id_t x, API_ADC_Channel_t y);

#ifdef __cplusplus
}
#endif

#endif /* __API_ADC_H */

/* Enroll 注册层，负责把板级资源注册到 BSP */
#include "Enroll.h"

/*系统sys层*/
#include "Delay.h"

/*API层 MCU片内外设*/
#include "usart.h"
#include "tim.h"
#include "pwm.h"
#include "adc.h"

/*app应用层*/
#include "My_Usart/My_Usart.h"
#include "My_I2c/My_I2c.h"
#include "My_SPI/My_SPI.h"
#include "Control_Task/Control_Task.h"

/*BSP硬件抽象层*/
#include "LED.h"
#include "KEY.h"
#include "OLED.h"

int main(void)
{
/* 板子注册层初始化 */
	Enroll_LED_Init(LED_LOW); 				/* LED 资源注册 */
	Enroll_KEY_Init();						/*  KEY 资源注册   */
	Enroll_USART_Init(API_USART1, 115200U);	/* USART 资源注册 */
	Enroll_USART_RegisterIrqHandler(Control_Task_USART_Callback); 	/* 串口中断回调注册 */
	Enroll_TIM_RegisterIrqHandler(Control_Task_TIM_Callback); 		/* 定时器中断回调注册 */
	/* PWM资源注册: G3507  TIM1 -> 10kHz 400，8  | 103 TIM2 -> 1kHz 100，720 | 407 TIM1 -> 50Hz 4000，840*/
	// Enroll_PWM_Init(API_PWM_TIM1, 400U - 1U, 8U - 1U);
	Enroll_ADC_Init(API_ADC1); /* ADC0资源注册 */

	Enroll_I2C_Register();					/*  I2C 资源注册   */
	Enroll_SPI_Register();					/*  SPI 资源注册   */
	Enroll_OLED_Register();				/* OLED SPI 控制脚注册 */

/*API层 MCU片内外设初始化*/	
	API_TIM_Init(API_TIM1, 1U); /* 定时器初始化：API_TIM1，每 1ms 触发一次更新中断 */

/* 通信协议初始化 */
	MyI2C_Init();							/* 软件 I2C 初始化 */
	App_I2C_ScanOnce();						/* 开机执行一次 I2C 扫描 */
	MySPI_Init();							/* 软件 SPI 初始化 */
	App_SPI_TestOnce();						/* 开机执行一次 SPI 测试 */
 
/*BSP硬件抽象层初始化*/
	OLED_Init(OLED_IF_SPI);		/* OLED_IF_I2C(4针) / OLED_IF_SPI(7针) */

	while (1)
	{
/* LED和延时测试 */
		// LED_Control(LED1, LED_HIGH);
		// Delay_ms(500U);
		// LED_Control(LED1, LED_LOW);
		// Delay_ms(500U);

/* KEY测试 Key 0变成1 */
		// key_Get();

/* 串口测试 */
		// usart_printf(USART1, "Timer_Bsp_t: %lu\r\n", Timer_Bsp_t);

/* PWM测试 */
		// API_PWM_Setcom(API_PWM_TIM1, API_PWM_CH1, 100U);
		// API_PWM_Setcom(API_PWM_TIM1, API_PWM_CH2, 300U);

/* ADC测试 */
		// uint16_t adc2 = API_ADC_GetValue(API_ADC1, API_ADC_CH2);
		// uint16_t adc5 = API_ADC_GetValue(API_ADC2, API_ADC_CH5);

/*OLED测试*/
		OLED_Printf(0, 0, OLED_8X16, "%d", Timer_Bsp_t);
		OLED_Update();
	}
}

/*Enroll注册层，负责把板级资源注册到 BSP*/
#include "Enroll.h"

/*系统sys层*/
#include "Delay.h"
#include "sys.h"

/*API层 MCU片内外设*/
#include "tim.h"
#include "usart.h"
#include "pwm.h"
#include "adc.h"

/*app应用层*/
#include "My_I2c/My_I2c.h"
#include "My_SPI/My_SPI.h"
#include "My_Usart/My_Usart.h"
#include "Control_Task/Control_Task.h"

/*BSP硬件抽象层*/
#include "OLED.h"
#include "MPU6050.h"
#include "MPU6050_Int.h"
#include "QMC5883P.h"
#include "BMP280.h"
#include "NRF24L01.h"

int main(void)	
{
	Enroll_LED_Init(LED_LOW); 				/*  LED 资源注册   */
	Enroll_KEY_Init();						/*  KEY 资源注册   */
	Enroll_USART_Register(); 				/*  USART 资源注册 */
	Enroll_I2C_Register();					/*  I2C 资源注册   */
	Enroll_SPI_Register();					/*  SPI 资源注册   */
	Enroll_OLED_Register();				/*  OLED SPI控制引脚资源注册 */
	Enroll_NRF24L01_Register();			/*  NRF24L01 CE 控制引脚资源注册 */
	Enroll_PWM_Register();					/*  PWM 资源注册   */
	Enroll_ADC_Register();					/*  ADC 资源注册   */
	SYS_Init();								/* 系统层初始化 */
	Enroll_MPU6050_EXTI_Register();		    /* 注册 MPU6050 外部中断：PE7/EXTI7/上升沿 */

/*API层 MCU片内外设初始化*/	
	API_TIM_Init(API_TIM3, 1U); 			/* 定时器初始化：API_TIM3，每 1ms 触发一次更新中断 */
	API_USART_Init(API_USART1, 115200); 	/* 串口初始化：API_USART1，波特率 115200 */

	// API_PWM_Init(API_PWM_TIM2, 100-1, 720-1);	/* 103_PWM初始化 */
	API_PWM_Init(API_PWM_TIM1, 4000-1, 840-1);	/* 407_PWM初始化 */
	API_ADC_Init(API_ADC1);					/* ADC1 初始化*/

	MyI2C_Init();							/* 软件 I2C 初始化 */
	App_I2C_ScanOnce();						/* 开机执行一次 I2C 扫描 */
	MySPI_Init();							/* 软件 SPI 初始化 */
	App_SPI_TestOnce();						/* 开机执行一次 SPI 测试 */

/*BSP硬件抽象层初始化*/
  	MPU_Init();			/* 初始化 MPU6050 */
	mpu_dmp_init(); 	/* 初始化 MPU6050 DMP */
	QMC_Init();			/* 初始化 QMC5883P */
	BMP280Init();		/* 初始化 BMP280 */
	NRF24L01_Init();	/* 初始化 NRF24L01 */
	App_NRF24L01_TestOnce();	/* 开机执行一次 NRF24L01 通信测试 */
	OLED_Init(OLED_IF_I2C);		/* OLED_IF_I2C(4针) / OLED_IF_SPI(7针) */
	
	while(1)
	{

/*I2C测试-9轴*/
		mpu_angle();
		// Angle_XY = QMC_Data();
		// alt = BMP_Data();

/*OLED测试*/
		OLED_Printf(0, 0, OLED_8X16, "%d", Timer_Bsp_t);
		OLED_Printf(0, 16, OLED_8X16, "P:%.1f",Pitch);
		OLED_Printf(0, 32, OLED_8X16, "R:%.1f",Roll);
		OLED_Printf(0, 48, OLED_8X16, "Y:%.1f",Yaw);
		// OLED_Printf(0, 32, OLED_8X16, "A:%.1f", Angle_XY);
		// OLED_Printf(0, 48, OLED_8X16, "alt: %.1f", alt);
		OLED_Update();

/*ADC测试*/
		// AD2 = API_ADC_GetValue(API_ADC1, API_ADC_CH2);
		// AD3 = API_ADC_GetValue(API_ADC1, API_ADC_CH3);

/*PWM测试*/
		// API_PWM_Setcom(API_PWM_TIM1, API_PWM_CH1, 1000U); 
		// API_PWM_Setcom(API_PWM_TIM1, API_PWM_CH2, 2000U);

/*printf测试*/
		if (print_task_flag)
		{
			print_task_flag = 0;

			// usart_printf(USART1,"AD2: %d, AD3: %d\r\n", AD2, AD3);
			// usart_printf(USART1,"Pitch:%.1f, Roll:%.1f, Yaw:%.1f,\r\n",Pitch,Roll,Yaw);
			// printf("Angle_XY: %.1f,\r\n", Angle_XY);
			// printf("alt: %.1f \r\n", alt);
		}
	}
}

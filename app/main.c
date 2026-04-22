/*Enroll注册层，负责把板级资源注册到 BSP*/
#include "Enroll.h"

/*系统sys层*/
#include "Delay.h"
#include "sys.h"

/*API层 MCU片内外设*/
#include "tim.h"
#include "usart.h"

/*app应用层*/
#include "My_I2c/My_I2c.h"
#include "My_Usart/My_Usart.h"
#include "Control_Task/Control_Task.h"

/*BSP硬件抽象层*/
#include "OLED.h"
#include "MPU6050.h"
#include "MPU6050_Int.h"

int main(void)	
{
	Enroll_LED_Init(LED_LOW); 				/*  LED 资源注册   */
	Enroll_KEY_Init();						/*  KEY 资源注册   */
	Enroll_USART_Register(); 				/*  USART 资源注册 */
	Enroll_I2C_Register();					/*  I2C 资源注册   */
	SYS_Init();								/* 系统层初始化 */
	Enroll_MPU6050_EXTI_Register();		/* 注册 MPU6050 外部中断：PE7/EXTI7/上升沿 */

/*API层 MCU片内外设初始化*/	
	API_TIM_Init(API_TIM3, 1U); 			/* 定时器初始化：API_TIM3，每 1ms 触发一次更新中断。 */
	API_USART_Init(API_USART1, 115200); 	/* 串口初始化：API_USART1，波特率 115200 */
	MyI2C_Init();							/* 软件 I2C 初始化 */
	App_I2C_ScanOnce();						/* 开机执行一次 I2C 扫描 */

/*BSP硬件抽象层初始化*/
  	MPU_Init();			/* 初始化MPU6050 */
	mpu_dmp_init(); 	/* 初始化MPU6050 DMP */
	// OLED_Init();		/* OLED 初始化 */
	
	while(1)
	{
		// OLED_Printf(0, 0, OLED_8X16, "%d", Timer_Bsp_t);
		// OLED_Update();

		mpu_angle();

		if (print_task_flag)
		{
			print_task_flag = 0;
			// printf("Pitch:%.1f, Roll:%.1f, Yaw:%.1f,\r\n",Pitch,Roll,Yaw);
			usart_printf(USART1,"Pitch:%.1f, Roll:%.1f, Yaw:%.1f,\r\n",Pitch,Roll,Yaw);
			// printf("Gyrox:%hd, Gyroy:%hd, Gyroz:%hd,\r\n",gyrox,gyroy,gyroz);
			// usart_printf(USART1, "USART_1_RX: %c\r\n", USART_1_RX);
			// printf("Time: %d\r\n", Timer_Bsp_t);
		}
	}
}

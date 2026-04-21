#include "Enroll.h" // 注册层头文件，负责把板级资源注册到 BSP

#include "Delay.h"
#include "tim.h"
#include "usart.h"

#include "OLED.h"
#include "MPU6050.h"
#include "My_Usart/My_Usart.h"
#include "My_I2c/My_I2c.h"
#include "Control_Task/Control_Task.h"

float Pitch, Roll, Yaw;	        // Pitch：俯仰角，Roll：横滚角，Yaw：偏航角
short gyrox, gyroy, gyroz;        // 角速度,x轴、y轴、z轴
uint8_t dmp_init_res;             // DMP 初始化返回码
uint8_t dmp_get_res;              // DMP 取数返回码

int main(void)	
{
	Enroll_LED_Init(LED_LOW); 				/*LED 资源注册*/
	Enroll_KEY_Init();					/* KEY 资源注册 */
	Enroll_USART_Register(); 				/* USART 资源注册 */
	Enroll_I2C_Register();					/* I2C 资源注册 */

	API_USART_Init(API_USART1, 115200); 	/* 串口初始化：API_USART1，波特率 115200。 */
	MyI2C_Init();						/* 软件 I2C 初始化 */
	App_I2C_ScanOnce();					/* 开机执行一次 I2C 扫描 */
  	MPU_Init(); // 初始化MPU6050
	dmp_init_res = mpu_dmp_init(); // 初始化MPU6050 DMP
	OLED_Init();						/* OLED 初始化 */
	API_TIM_Init(API_TIM3, 1U); 			/* 定时器初始化：API_TIM3，每 1ms 触发一次更新中断。 */
	
	while(1)
	{

		// Enroll_LED_Control(LED1, LED_HIGH);
		// Delay_ms(500);
		// Enroll_LED_Control(LED1, LED_LOW);
		// Delay_ms(500);

		// OLED_Clear();
		// OLED_Printf(0, 0, OLED_8X16, "%d", Timer_Bsp_t);
		// OLED_Update();

		dmp_get_res = mpu_dmp_get_data(&Pitch,&Roll,&Yaw);	    // 读取角度
		// MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);  // 读取角速度

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

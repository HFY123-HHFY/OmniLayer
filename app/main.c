/* Enroll 注册层，负责把板级资源注册到 BSP */
#include "Enroll.h"

/*系统sys层*/
#include "sys.h"
#include "Delay.h"

/*API层 MCU片内外设*/
#include "usart.h"
#include "tim.h"
#include "pwm.h"
#include "adc.h"
#include "Encoder.h"

/*app应用层*/
#include "My_Usart/My_Usart.h"
#include "My_I2c/My_I2c.h"
#include "My_SPI/My_SPI.h"
#include "PID/PID.h"
#include "Control/Control.h"
#include "Control_Task/Control_Task.h"

/*BSP硬件抽象层*/
#include "LED.h"
#include "KEY.h"
#include "OLED.h"
#include "MPU6050.h"
#include "MPU6050_Int.h"
#include "Control.h"
#include "TB6612.h"

int main(void)
{
/* 系统时钟配置初始化 */
	SYS_Init();
/* 注册层：注册相关资源，登记资源映射 */
	Enroll_USART_Register();				/* USART 资源注册 */
	Enroll_PWM_Register();					/* PWM 资源注册 */
	Enroll_ADC_Register();					/* ADC 资源注册 */
	Enroll_TIM_Register();					/* TIM 资源注册 */
	Enroll_I2C_Register();					/* I2C 资源注册 */
	Enroll_SPI_Register();					/* SPI 资源注册 */
	Enroll_LED_Register();					/* LED 资源注册 */
	Enroll_KEY_Register();					/* KEY 资源注册 */
	Enroll_MPU6050_Register();				/* MPU6050 INT 资源注册 */
	Enroll_OLED_Register();					/* OLED SPI 控制脚注册 */
	Enroll_TB6612_Register();				/* TB6612 资源注册 */
	Enroll_Encoder_Register();				/* 编码器 资源注册 */

	/* 注册后绑定中断回调*/
	Enroll_USART_RegisterIrqHandler(Control_Task_USART_Callback); /* USART 中断回调注册 */
	Enroll_TIM_RegisterIrqHandler(Control_Task_TIM_Callback);	   /* TIM 中断回调注册 */

/* 初始化层：初始化相关外设，启动硬件功能 */
	API_USART_Init(API_USART1, 115200U); // 初始化 USART1，波特率 115200
	API_USART_Init(API_USART2, 115200U); // 初始化 USART2，波特率 115200
	API_USART_Init(API_USART3, 115200U); // 初始化 USART3，波特率 115200
	/* PWM 初始化示例:
	 * G3507: API_PWM_TIM1 -> 10kHz, ARR=400-1, PSC=8-1
	 * F103 : API_PWM_TIM2 -> 1kHz,  ARR=100-1, PSC=720-1
	 * F407 : API_PWM_TIM1 -> 50Hz,  ARR=4000-1, PSC=840-1
	 */
	API_PWM_Init(API_PWM_TIM1, 400U - 1U, 8U - 1U);
	API_ADC_Init(API_ADC1); // 初始化 ADC1
	API_TIM_Init(API_TIM1, 1U); /* 定时器初始化：API_TIM1，每 1ms 触发一次更新中断 */

/* 通信协议初始化 */
	MyI2C_Init();						/* 软件 I2C 初始化 */
	MySPI_Init();						/* 软件 SPI 初始化 */
	// App_I2C_ScanOnce();					/* 开机执行一次 I2C 扫描 */
	// App_SPI_TestOnce();				/* 开机执行一次 SPI 测试 */

/*BSP硬件抽象层初始化*/
	LED_Init(LED_LOW); // 初始化LED-低电平
	KEY_Init(); // 初始化按键
	OLED_Init(OLED_IF_SPI);		 /* OLED_IF_I2C(4针) / OLED_IF_SPI(7针) */
	MPU_Init();
	// uint8_t mpu6050_dma_int = mpu_dmp_init();
	// usart_printf(USART1, "mpu6050_dma_int= %d\r\n", mpu6050_dma_int);
	TB6612_Init(); /* TB6612 电机驱动初始化 */
	API_Encoder_Init(API_ENCODER_1); /* 编码器 1 初始化 */
	API_Encoder_Init(API_ENCODER_2); /* 编码器 2 初始化 */

/* PID控制器初始化 */
	PID_Speed_Init(); /* 速度环初始化 */
	// PID_EncoderSpeed_Set(&speed_loop, 1.5f, 40.0f, 0.0f, -50.0f); /* 设置速度环 PID 参数与目标值 */

	while (1)
	{
/* LED和延时测试 */
		// LED_Control(LED1, LED_HIGH);
		// LED_Turn(LED2, 500); /* LED1 翻转闪烁，周期 500ms */
		// LED_Control(Buzzer1, LED_HIGH);

/* KEY测试 Key 0变成1 */
		key_Get();
		if (Key == 1U)
		{
			LED_Control(LED1, LED_HIGH);
		}
		if (Key == 2U)
		{
			LED_Control(LED2, LED_HIGH);
			PID_EncoderSpeed_Set(&speed_loop, 1.5f, 40.0f, 0.0f, 80.0f); /* 设置速度环 PID 参数与目标值 */
		}
		if (Key == 3U)
		{
			LED_Control(LED3, LED_HIGH);
			PID_Reset(&speed_loop.left);
			PID_Reset(&speed_loop.right);
			PID_EncoderSpeed_Set(&speed_loop, 0.0f, 0.0f, 0.0f, 0.0f); /* 设置速度环 PID 参数与目标值 */
		}
		if (Key == 4U)
		{
			LED_Control(LED1, LED_LOW);
			LED_Control(LED2, LED_LOW);
			LED_Control(LED3, LED_LOW);
			PID_EncoderSpeed_Set(&speed_loop, 1.5f, 40.0f, 0.0f, -100.0f); /* 设置速度环 PID 参数与目标值 */
		}

/* 串口测试 */
		// usart_printf(USART1, "Timer_Bsp_t: %lu\r\n", Timer_Bsp_t);

/* PWM测试 */
		// API_PWM_Setcom(API_PWM_TIM1, API_PWM_CH2, 300U);

/* ADC测试 */
		// uint16_t adc2 = API_ADC_GetValue(API_ADC1, API_ADC_CH2);
		// uint16_t adc5 = API_ADC_GetValue(API_ADC2, API_ADC_CH5);

/* TB6612测试 */
		// TB6612_SetSpeed(100, 100);

/* MPU6050测试 */
		// mpu_angle();

 /* 串级PID控制 - 2ms 姿态环*/
		if (pid_task_flag != 0U)   // 500Hz 姿态环
		{
			pid_task_flag = 0U;
			// PID_Pitch_Roll_Combined(Pitch, Roll);
		}

/* 读编码器 + 速度环 PID（20ms 周期，与 Encoder_flag 同步） */
		if (Encoder_flag != 0U)
		{
			Encoder_flag = 0U;
			Encoder1_Speed = API_Encoder_GetSpeed(API_ENCODER_1);
			Encoder2_Speed = API_Encoder_GetSpeed(API_ENCODER_2);

			/* 速度环 */
			PID_Speed_Control((float)(Encoder1_Speed), (float)Encoder2_Speed);
		}

/* 串口数据打印 */
		if (print_task_flag != 0U)
		{
			print_task_flag = 0U;
			usart_printf(USART1, "1=%d, 2=%d, Lout=%d, Rout=%d\r\n", Encoder1_Speed, Encoder2_Speed, (int)speed_loop.left.output, (int)speed_loop.right.output);
			usart_printf(USART1, "P=%.1f, I=%.1f, D=%.1f\r\n", (double)speed_loop.left.P_out, (double)speed_loop.left.I_out, (double)speed_loop.left.D_out);
			// usart_printf(USART1, "key: %lu\r\n", Key);
			// usart_printf(USART1, "Timer_Bsp_t: %lu\r\n", Timer_Bsp_t);
			// usart_printf(USART1, "Pitch=%.2f Roll=%.2f Yaw=%.2f\r\n", Pitch, Roll, Yaw);
			// usart_printf(USART2, "Pitch=%.2f Roll=%.2f Yaw=%.2f\r\n", Pitch, Roll, Yaw); /* 无线串口 */
			// usart_printf(USART1, "GyroX=%d GyroY=%d GyroZ=%d\r\n", gyrox, gyroy, gyroz);
		}

/* OLED测试 */
		OLED_Clear();
		OLED_Printf(0, 0, OLED_8X16, "%d", Timer_Bsp_t);
		OLED_Printf(0, 16, OLED_8X16, "%.1f  %.1f  %.1f", Pitch, Roll, Yaw);
		OLED_Printf(0, 32, OLED_8X16, "L %d  R %d", Encoder1_Speed, Encoder2_Speed);
		OLED_Printf(0, 48, OLED_8X16, "Lo %d  Ro %d", (int)speed_loop.left.output, (int)speed_loop.right.output);
		OLED_Update();
	}
}

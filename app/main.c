/* Enroll 注册层，负责把板级资源注册到 BSP */
#include "Enroll.h"

/*系统sys层*/
#include "Delay.h"

/*API层 MCU片内外设*/
#include "usart.h"

/*app应用层*/
#include "My_Usart/My_Usart.h"

/*BSP硬件抽象层*/
#include "LED.h"

int main(void)
{
	/* 板子注册层初始化 */
	Enroll_LED_Init(LED_LOW); /* LED 资源注册 */
	Enroll_USART_Init(API_USART1, 115200U);

	   while (1)
	   {
			usart_printf(USART1, "hhh\r\n");
			// printf("hhh\r\n");
			Enroll_LED_Control(LED1, LED_HIGH);
			Delay_ms(1000U);
			Enroll_LED_Control(LED1, LED_LOW);
			Delay_ms(1000U);
	   }
}

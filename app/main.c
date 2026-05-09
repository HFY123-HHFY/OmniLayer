/* Enroll 注册层，负责把板级资源注册到 BSP */
#include "Enroll.h"
#include "LED.h"
#include "Delay.h"

int main(void)
{
	/* 板子注册层初始化 */
	Enroll_LED_Init(LED_LOW); /* LED 资源注册 */

	while (1)
	{
		Enroll_LED_Control(LED1, LED_HIGH);
		Delay_ms(200U);
		Enroll_LED_Control(LED1, LED_LOW);
		Delay_ms(200U);
	}
}

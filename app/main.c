#include "Enroll.h"
#include "Delay.h"

int main(void)	
{
	/* 通过 Enroll 层完成 LED 资源注册，并统一初始化输出电平。 */
	Enroll_LED_Init(LED_LOW);
	
	while(1)
	{
		Enroll_LED_Control(LED1, LED_HIGH);
		Delay_ms(1000);
		Enroll_LED_Control(LED1, LED_LOW);
		Delay_ms(1000);
	}
}

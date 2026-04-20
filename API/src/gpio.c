#include "gpio.h"

#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
#include "f103_gpio.h"
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
#include "f407_gpio.h"
#else
#error "Unsupported ENROLL_MCU_TARGET for API GPIO backend."
#endif

void API_GPIO_InitOutput(void *port, uint16_t pin)
{
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	F103_GPIO_InitOutput(port, pin);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	F407_GPIO_InitOutput(port, pin);
#else
	(void)port;
	(void)pin;
#endif
}

void API_GPIO_InitInput(void *port, uint16_t pin)
{
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	F103_GPIO_InitInput(port, pin);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	F407_GPIO_InitInput(port, pin);
#else
	(void)port;
	(void)pin;
#endif
}

void API_GPIO_Write(void *port, uint16_t pin, uint8_t level)
{
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	F103_GPIO_Write(port, pin, level);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	F407_GPIO_Write(port, pin, level);
#else
	(void)port;
	(void)pin;
	(void)level;
#endif
}

uint8_t API_GPIO_Read(void *port, uint16_t pin)
{
#if (ENROLL_MCU_TARGET == ENROLL_MCU_F103)
	return F103_GPIO_Read(port, pin);
#elif (ENROLL_MCU_TARGET == ENROLL_MCU_F407)
	return F407_GPIO_Read(port, pin);
#else
	(void)port;
	(void)pin;
	return 0U;
#endif
}

#ifndef __G3507_GPIO_H
#define __G3507_GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GPIO 输出初始化：配置 port 的 pin 为推挽输出。 */
void G3507_GPIO_InitOutput(void *port, uint32_t pin);
/* GPIO 输入初始化：配置 port 的 pin 为浮空输入。 */
void G3507_GPIO_InitInput(void *port, uint32_t pin);
/* GPIO 输入上拉初始化：配置 port 的 pin 为上拉输入。 */
void G3507_GPIO_InitInputPullUp(void *port, uint32_t pin);
/* GPIO 写电平：level 非 0 写高，0 写低。 */
void G3507_GPIO_Write(void *port, uint32_t pin, uint8_t level);
/* GPIO 读电平：返回 1 表示高电平，0 表示低电平。 */
uint8_t G3507_GPIO_Read(void *port, uint32_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __G3507_GPIO_H */

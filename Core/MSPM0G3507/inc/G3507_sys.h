#ifndef __G3507_SYS_H
#define __G3507_SYS_H

#include <stdint.h>

void G3507_SYS_Init(void);
uint32_t G3507_SYS_GetMclkHz(void);
uint32_t G3507_SYS_GetBusClkHz(void);
uint32_t G3507_SYS_GetResetCause(void);

#endif /* __G3507_SYS_H */

#ifndef __G3507_PINMUX_H
#define __G3507_PINMUX_H

#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "ti/driverlib/dl_gpio.h"

/*
 * G3507_pinmux.h
 * 这里集中维护“GPIO 口 + 32位 pin mask + IOMUX_PINCM”的对应关系。
 * 仅供查表用途，不参与代码逻辑。
 */

// PA口
#define A0   (IOMUX_PINCM1)
#define A1   (IOMUX_PINCM2)
#define A2   (IOMUX_PINCM7)
#define A3   (IOMUX_PINCM8)
#define A4   (IOMUX_PINCM9)
#define A5   (IOMUX_PINCM10)
#define A6   (IOMUX_PINCM11)
#define A7   (IOMUX_PINCM14)
#define A8   (IOMUX_PINCM19)
#define A9   (IOMUX_PINCM20)
#define A10  (IOMUX_PINCM21)
#define A11  (IOMUX_PINCM22)
#define A12  (IOMUX_PINCM34)
#define A13  (IOMUX_PINCM35)
#define A14  (IOMUX_PINCM36)
#define A15  (IOMUX_PINCM37)
#define A16  (IOMUX_PINCM38)
#define A17  (IOMUX_PINCM39)
#define A18  (IOMUX_PINCM40)
#define A19  (IOMUX_PINCM41)
#define A20  (IOMUX_PINCM42)
#define A21  (IOMUX_PINCM46)
#define A22  (IOMUX_PINCM47)
#define A23  (IOMUX_PINCM53)
#define A24  (IOMUX_PINCM54)
#define A25  (IOMUX_PINCM55)
#define A26  (IOMUX_PINCM59)
#define A27  (IOMUX_PINCM60)
#define A28  (IOMUX_PINCM3)
#define A29  (IOMUX_PINCM4)
#define A30  (IOMUX_PINCM5)
#define A31  (IOMUX_PINCM6)

// PB口
#define B0   (IOMUX_PINCM12)
#define B1   (IOMUX_PINCM13)
#define B2   (IOMUX_PINCM15)
#define B3   (IOMUX_PINCM16)
#define B4   (IOMUX_PINCM17)
#define B5   (IOMUX_PINCM18)
#define B6   (IOMUX_PINCM23)
#define B7   (IOMUX_PINCM24)
#define B8   (IOMUX_PINCM25)
#define B9   (IOMUX_PINCM26)
#define B10  (IOMUX_PINCM27)
#define B11  (IOMUX_PINCM28)
#define B12  (IOMUX_PINCM29)
#define B13  (IOMUX_PINCM30)
#define B14  (IOMUX_PINCM31)
#define B15  (IOMUX_PINCM32)
#define B16  (IOMUX_PINCM33)
#define B17  (IOMUX_PINCM43)
#define B18  (IOMUX_PINCM44)
#define B19  (IOMUX_PINCM45)
#define B20  (IOMUX_PINCM48)
#define B21  (IOMUX_PINCM49)
#define B22  (IOMUX_PINCM50)
#define B23  (IOMUX_PINCM51)
#define B24  (IOMUX_PINCM52)
#define B25  (IOMUX_PINCM56)
#define B26  (IOMUX_PINCM57)
#define B27  (IOMUX_PINCM58)

#endif /* __G3507_PINMUX_H */

#ifndef __ENROLL_INTERNAL_H
#define __ENROLL_INTERNAL_H

/*
 * Enroll Internal 层职责：
 * 1) 提供 Enroll.c 专用的内部依赖；
 * 2) 存放只供注册实现使用的头文件与宏；
 * 3) 不建议被 App/其他模块直接 include。
 */

#include "Enroll.h"

/*
 * 内部实现依赖：
 * 这些头文件只服务于 Enroll.c 的注册实现，不属于对外接口。
 */
#include "OLED.h"
#include "exti.h"
#include "MPU6050_Int.h"

#include <stddef.h>

#endif /* __ENROLL_INTERNAL_H */

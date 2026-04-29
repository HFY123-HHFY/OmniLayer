#ifndef __PID_H
#define __PID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PID_ENABLE  (1U) /* PID使能 */
#define PID_DISABLE (0U) /* PID禁用 */

/*
 * PID 模式：
 * 1) 位置式：输出为当前时刻绝对控制量
 * 2) 增量式：输出为增量累加，适合部分执行器
 */
typedef enum
{
	PID_MODE_POSITION = 0,
	PID_MODE_INCREMENTAL = 1
} PID_Mode_t;

/*
 * PID 控制器对象
 */
typedef struct
{
	/* PID 参数 */
	float kp;
	float ki;
	float kd;

	/* 目标、实际、输出 */
	float Target;
	float Actual;
	float output;

	/* P/I/D 分项输出，便于在线调试 */
	float P_out;
	float D_out;
	float I_out;

	/* 当前误差、上一拍误差、上上拍误差 */
	float error0;
	float error1;
	float error2;
	/* 积分累加项（积分前的误差和） */
	float error_sum;

	/* 积分限幅与总输出限幅 */
	float Integral_max;
	float Out_max;

	/* 默认采样周期(s) */
	float dt;
	/* 死区阈值，|error| <= deadband 时按 0 处理 */
	float deadband;
	/* 积分分离阈值，|error| > threshold 时暂停积分 */
	float integral_separation;
	/* 微分低通系数 alpha，范围[0,1] */
	float d_lpf_alpha;
	/* 微分低通内部状态 */
	float d_filter_state;

	/* 使能开关 */
	uint8_t enable;
	/* 抗积分饱和开关 */
	uint8_t anti_windup;
	/* PID 模式 */
	uint8_t mode;
} PID_TypeDef;

/*
 * 串级 PID 组合对象
 * outer: 外环（角度/位置）
 * inner: 内环（角速度/速度）
 */
typedef struct
{
	PID_TypeDef* outer;
	PID_TypeDef* inner;
} PID_Cascade_t;

/*
 * 通用对称限幅。
 * 输入 value 将被限制到 [-max, max]。
 */
float Limit_Output(float value, float max);

/*
 * 初始化 PID（含默认安全参数）。
 * 用法示例：
 *   PID_TypeDef pid_pitch;
 *   PID_Init(&pid_pitch, 2.0f, 0.1f, 0.02f);
 */
void PID_Init(PID_TypeDef* pid, float kp, float ki, float kd);

/*
 * 复位 PID 内部状态（不改 kp/ki/kd 与配置）。
 */
void PID_Reset(PID_TypeDef* pid);

/*
 * PID 使能控制。
 * 关闭时会自动清状态，防止再次使能时瞬态冲击。
 */
void PID_Enable(PID_TypeDef* pid, uint8_t enable);

/*
 * 切换 PID 模式（位置式/增量式）。
 * 切换后会自动清状态。
 */
void PID_SetMode(PID_TypeDef* pid, PID_Mode_t mode);

/* 设置目标值。 */
void PID_SetTarget(PID_TypeDef* pid, float target);
/* 设置误差死区。 */
void PID_SetDeadband(PID_TypeDef* pid, float deadband);
/* 设置默认采样周期(s)。 */
void PID_SetSampleTime(PID_TypeDef* pid, float dt);
/* 设置输出限幅。 */
void PID_SetOutputLimit(PID_TypeDef* pid, float out_max);
/* 设置积分限幅。 */
void PID_SetIntegralLimit(PID_TypeDef* pid, float integral_max);
/* 设置积分分离阈值。 */
void PID_SetIntegralSeparation(PID_TypeDef* pid, float threshold);
/* 设置微分低通系数 alpha。 */
void PID_SetDerivativeLPF(PID_TypeDef* pid, float alpha);
/* 设置抗积分饱和开关。 */
void PID_SetAntiWindup(PID_TypeDef* pid, uint8_t enable);

/*
 * 设置 kp/ki/kd。
 */
void Set_PID(PID_TypeDef* pid, float kp, float ki, float kd);

/*
 * 使用 pid->dt 进行一次 PID 计算。
 */
float PID_Calc(PID_TypeDef* pid, float actual);

/*
 * 以显式 dt 进行一次 PID 计算。
 * 当任务周期不固定时，建议优先使用此接口。
 */
float PID_CalcDt(PID_TypeDef* pid, float actual, float dt);

/* 初始化串级 PID。 */
void PID_Cascade_Init(PID_Cascade_t* cascade, PID_TypeDef* outer, PID_TypeDef* inner);

/*
 * 串级 PID 一次计算：
 * 1) 外环计算得到 inner_target
 * 2) inner_target 作为内环目标，输出最终控制量
 */
float PID_Cascade_Calc(PID_Cascade_t* cascade,
					   float outer_actual,
					   float inner_actual,
					   float outer_dt,
					   float inner_dt);

#ifdef __cplusplus
}
#endif

#endif /* PID_H */

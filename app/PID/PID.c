#include "PID.h"
#include <math.h>

/* My_PID库 */

/* 内部绝对值函数。 */
static float pid_abs(float x)
{
	return (x >= 0.0f) ? x : -x;
}

/*
 * 通用限幅函数。
 * max <= 0 时不做限制，便于调试场景快速放开限幅。
 */
float Limit_Output(float value, float max)
{
	if (max <= 0.0f)
	{
		return value;
	}
	if (value > max)
	{
		return max;
	}
	if (value < -max)
	{
		return -max;
	}
	return value;
}

/*
 * PID 初始化。
 * 
 */
void PID_Init(PID_TypeDef* pid)
{
	if (pid == 0)
	{
		return;
	}

	pid->kp = 0.0f;
	pid->ki = 0.0f;
	pid->kd = 0.0f;

	pid->Target = 0.0f;
	pid->Actual = 0.0f;
	pid->output = 0.0f;

	pid->P_out = 0.0f;
	pid->D_out = 0.0f;
	pid->I_out = 0.0f;

	pid->error0 = 0.0f;
	pid->error1 = 0.0f;
	pid->error2 = 0.0f;
	pid->error_sum = 0.0f;

	pid->dt = 0.002f; /* 默认采样周期 */
	pid->deadband = 0.2f; /* 死区 */
	pid->integral_separation = 0.0f; /* 积分分离阈值 */
	pid->d_lpf_alpha = 0.35f; /* 微分低通滤波系数 */
	pid->d_filter_state = 0.0f; /* 微分低通滤波内部状态 */

	pid->enable = PID_ENABLE;  /* 设置PID使能开关 */
	pid->anti_windup = PID_ENABLE; /* 设置抗积分饱和开关 */
	pid->mode = (uint8_t)PID_MODE_POSITION; /* 设置PID模式 - 位置式 PID */
}

/* PID 限幅初始化 */
void PID_Init_WithLimit(PID_TypeDef* pid,float Integral_max, float Out_max)
{
	if (pid == 0)
	{
		return;
	}

	pid->Integral_max = pid_abs(Integral_max);
	pid->Out_max = pid_abs(Out_max);
}

/*
 * 复位运行时状态。
 * 不修改 PID 参数与配置项，适用于模式切换/解锁后重新接管。
 */
void PID_Reset(PID_TypeDef* pid)
{
	if (pid == 0)
	{
		return;
	}

	pid->Actual = 0.0f;
	pid->output = 0.0f;

	pid->P_out = 0.0f;
	pid->D_out = 0.0f;
	pid->I_out = 0.0f;

	pid->error0 = 0.0f;
	pid->error1 = 0.0f;
	pid->error2 = 0.0f;

	pid->error_sum = 0.0f;
	pid->d_filter_state = 0.0f;
}

/*
 * 使能控制。
 * 当关闭 PID 时自动清状态，防止残留积分导致“猛冲”。
 */
void PID_Enable(PID_TypeDef* pid, uint8_t enable)
{
	if (pid == 0)
	{
		return;
	}

	pid->enable = (enable != 0U) ? PID_ENABLE : PID_DISABLE;
	if (pid->enable == PID_DISABLE)
	{
		PID_Reset(pid);
	}
}

/* 切换 PID 模式并清状态。 */
void PID_SetMode(PID_TypeDef* pid, PID_Mode_t mode)
{
	if (pid == 0)
	{
		return;
	}
	pid->mode = (uint8_t)mode;
	PID_Reset(pid);
}

/* 设置目标值。 */
void PID_SetTarget(PID_TypeDef* pid, float target)
{
	if (pid == 0)
	{
		return;
	}
	pid->Target = target;
}

/* 设置死区阈值为deadband */
void PID_SetDeadband(PID_TypeDef* pid, float deadband)
{
	if (pid == 0)
	{
		return;
	}
	pid->deadband = (deadband >= 0.0f) ? deadband : -deadband;
}

/* 设置采样周期 */
void PID_SetSampleTime(PID_TypeDef* pid, float dt)
{
	if (pid == 0)
	{
		return;
	}
	if (dt > 0.0f)
	{
		pid->dt = dt;
	}
}

/* 设置输出限幅。 */
void PID_SetOutputLimit(PID_TypeDef* pid, float out_max)
{
	if (pid == 0)
	{
		return;
	}
	pid->Out_max = pid_abs(out_max);
}

/* 设置积分限幅。 */
void PID_SetIntegralLimit(PID_TypeDef* pid, float integral_max)
{
	if (pid == 0)
	{
		return;
	}
	pid->Integral_max = pid_abs(integral_max);
}

/* 设置积分分离阈值。 */
void PID_SetIntegralSeparation(PID_TypeDef* pid, float threshold)
{
	if (pid == 0)
	{
		return;
	}
	pid->integral_separation = pid_abs(threshold);
}

/* 设置微分低通系数 范围0-1 越小滤波越强（响应变慢）常用0.1~0.5 平滑微分信号 */
void PID_SetDerivativeLPF(PID_TypeDef* pid, float alpha)
{
	if (pid == 0)
	{
		return;
	}

	if (alpha < 0.0f)
	{
		alpha = 0.0f;
	}
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}

	pid->d_lpf_alpha = alpha;
}

/* 设置抗积分饱和开关 根据执行器是否饱和来决定是否继续积分 */
void PID_SetAntiWindup(PID_TypeDef* pid, uint8_t enable)
{
	if (pid == 0)
	{
		return;
	}
	pid->anti_windup = (enable != 0U) ? PID_ENABLE : PID_DISABLE;
}

/* 更新 PID 三参数。 */
void Set_PID(PID_TypeDef* pid, float kp, float ki, float kd)
{
	if (pid == 0)
	{
		return;
	}

	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;
}

/*
 * 位置式 PID 计算。
 * 特性：
 * 1) 死区
 * 2) 积分分离
 * 3) 积分限幅
 * 4) 微分低通
 * 5) 抗积分饱和
 */
static float pid_calc_position(PID_TypeDef* pid, float actual, float dt)
{
	float error; //当前误差
	float derivative_raw; //原始微分
	float output_unsat; //未限幅输出
	/* limited_output: 执行限幅后的最终输出。 */
	float limited_output;
	/* should_integrate: 积分分离开关，1允许积分，0暂停积分。 */
	float should_integrate = 1.0f;

	/* 记录反馈值并计算误差。 */
	pid->Actual = actual;
	error = pid->Target - pid->Actual;

	/* 死区处理：误差足够小时按 0，减少抖动。 */
	if ((pid->deadband > 0.0f) && (pid_abs(error) <= pid->deadband))
	{
		error = 0.0f;
	}

	/* 积分分离：偏差过大时暂不积分，避免积分在大扰动下累积过快。 */
	if ((pid->integral_separation > 0.0f) && (pid_abs(error) > pid->integral_separation))
	{
		should_integrate = 0.0f;
	}

	/* 积分项更新与积分限幅。 */
	if ((should_integrate > 0.5f) && (pid->ki != 0.0f))
	{
		pid->error_sum += error * dt;
		pid->error_sum = Limit_Output(pid->error_sum, pid->Integral_max);
	}

	/* 误差微分并经过一阶低通，降低 D 项噪声敏感。 */
	derivative_raw = (dt > 0.0f) ? ((error - pid->error0) / dt) : 0.0f;
	pid->d_filter_state = (pid->d_lpf_alpha * derivative_raw) +
						  ((1.0f - pid->d_lpf_alpha) * pid->d_filter_state);

	/* 计算 P/I/D 分项。 */
	pid->P_out = pid->kp * error;
	pid->I_out = pid->ki * pid->error_sum;
	pid->D_out = pid->kd * pid->d_filter_state;

	/* 合成总输出并执行输出限幅。 */
	output_unsat = pid->P_out + pid->I_out + pid->D_out;
	limited_output = Limit_Output(output_unsat, pid->Out_max);

	/*
	 * 抗积分饱和：
	 * 当输出已饱和且误差方向会让饱和更严重时，撤销本次积分更新。
	 */
	if ((pid->anti_windup == PID_ENABLE) && (output_unsat != limited_output) && (pid->ki != 0.0f))
	{
		if (((output_unsat > pid->Out_max) && (error > 0.0f)) ||
			((output_unsat < -pid->Out_max) && (error < 0.0f)))
		{
			pid->error_sum -= error * dt;
			pid->error_sum = Limit_Output(pid->error_sum, pid->Integral_max);
			pid->I_out = pid->ki * pid->error_sum;
			output_unsat = pid->P_out + pid->I_out + pid->D_out;
			limited_output = Limit_Output(output_unsat, pid->Out_max);
		}
	}

	/* 更新误差历史，供下次微分/增量计算使用。 */
	pid->error2 = pid->error1;
	pid->error1 = pid->error0;
	pid->error0 = error;

	/* 写回最终输出。 */
	pid->output = limited_output;
	return pid->output;
}

/*
 * 增量式 PID 计算。Δu(k) = Kp * (e(k)-e(k-1)) + Ki * e(k) * T + Kd * (e(k) - 2e(k-1) + e(k-2)) / T
 * 输出：累加后的控制量 pid->output（绝对量）。
 */
static float pid_calc_incremental(PID_TypeDef* pid, float actual, float dt)
{
	float error; //当前误差
	float delta_u; //本次增量输出
	float derivative_inc; //误差的二阶差分
	float ki_term; //本次积分增量

	/* 误差计算。 */
	pid->Actual = actual;
	error = pid->Target - pid->Actual;

	/* 死区处理。 */
	if ((pid->deadband > 0.0f) && (pid_abs(error) <= pid->deadband))
	{
		error = 0.0f;
	}

	/* 使用离散二阶差分近似误差变化率，并做低通。 */
	derivative_inc = (dt > 0.0f) ? ((error - (2.0f * pid->error0) + pid->error1) / dt) : 0.0f;
	pid->d_filter_state = (pid->d_lpf_alpha * derivative_inc) +
						  ((1.0f - pid->d_lpf_alpha) * pid->d_filter_state);
	/* 增量式中 I 项是每拍增量。 */
	ki_term = pid->ki * error * dt;

	/* 增量合成：Δu = Kp*Δe + Ki*e*dt + Kd*Δde */
	delta_u = (pid->kp * (error - pid->error0)) + ki_term + (pid->kd * pid->d_filter_state);

	/* 输出累加并限幅 */
	pid->output += delta_u;
	pid->output = Limit_Output(pid->output, pid->Out_max);

	/* 记录分项量用于观测调试 */
	pid->P_out = pid->kp * error;
	pid->I_out += ki_term;
	pid->I_out = Limit_Output(pid->I_out, pid->Integral_max);
	pid->D_out = pid->kd * pid->d_filter_state;

	/* 更新历史误差。 */
	pid->error2 = pid->error1;
	pid->error1 = pid->error0;
	pid->error0 = error;

	return pid->output;
}

/*
 * 显式 dt 的统一入口。
 * 建议任务周期抖动明显时使用该接口。
 */
float PID_CalcDt(PID_TypeDef* pid, float actual, float dt)
{
	/* 防御式判空。 */
	if (pid == 0)
	{
		return 0.0f;
	}

	/* 禁用时直接返回 0 控制量。 */
	if (pid->enable == PID_DISABLE)
	{
		return 0.0f;
	}

	/* dt 非法时回退到对象默认采样周期。 */
	if (dt <= 0.0f)
	{
		dt = (pid->dt > 0.0f) ? pid->dt : 0.001f;
	}

	/* 按当前模式选择对应算法。 */
	if (pid->mode == (uint8_t)PID_MODE_INCREMENTAL)
	{
		return pid_calc_incremental(pid, actual, dt);
	}

	return pid_calc_position(pid, actual, dt);
}

/* 使用默认 dt 的快捷入口。 */
float PID_Calc(PID_TypeDef* pid, float actual)
{
	if (pid == 0)
	{
		return 0.0f;
	}
	return PID_CalcDt(pid, actual, pid->dt);
}

/* 初始化串级 PID 对象 */
void PID_Cascade_Init(PID_Cascade_t* cascade, PID_TypeDef* outer, PID_TypeDef* inner)
{
	if (cascade == 0)
	{
		return;
	}

	cascade->outer = outer;
	cascade->inner = inner;
}

/*
 * 串级 PID 计算。
 * 调用流程：
 * 1) 先算外环，外环输出作为内环目标
 * 2) 再算内环，内环输出即执行器控制量
 */
float PID_Cascade_Calc(PID_Cascade_t* cascade,
					   float outer_actual,
					   float inner_actual,
					   float outer_dt,
					   float inner_dt)
{
	/* inner_target: 外环输出，作为内环目标输入。 */
	float inner_target;

	if ((cascade == 0) || (cascade->outer == 0) || (cascade->inner == 0))
	{
		return 0.0f;
	}

	/* 第一步：外环输出“期望角速度/速度”等内环目标。 */
	inner_target = PID_CalcDt(cascade->outer, outer_actual, outer_dt);
	/* 第二步：把外环结果写入内环目标。 */
	cascade->inner->Target = inner_target;
	/* 第三步：内环计算最终执行器控制量。 */
	return PID_CalcDt(cascade->inner, inner_actual, inner_dt);
}

/* 编码器速度环初始化。Out_max 须匹配 TB6612_MAX_DUTY（400），否则反积分饱和失效。 */
void PID_EncoderSpeed_Init(PID_EncoderSpeed_t* speed)
{
	if (speed == 0)
	{
		return;
	}

	PID_Init(&speed->left);
	PID_Init(&speed->right);

	/*
	 * Integral_max 限制的是 error_sum，不是 I 贡献量。
	 * I_out = ki × error_sum。所以 max_I_out = ki × Integral_max。
	 * Integral_max 要给足够余量，否则稳态误差永远补不回来。
	 */
	PID_Init_WithLimit(&speed->left,  5000.0f, 400.0f);
	PID_Init_WithLimit(&speed->right, 5000.0f, 400.0f);
}

/* 设置编码器速度环参数与目标。 */
void PID_EncoderSpeed_Set(PID_EncoderSpeed_t* speed,
				  float kp,
				  float ki,
				  float kd,
				  float target)
{
	if (speed == 0)
	{
		return;
	}

	Set_PID(&speed->left, kp, ki, kd);
	Set_PID(&speed->right, kp, ki, kd);

	PID_SetTarget(&speed->left, target);
	PID_SetTarget(&speed->right, target);
}

/* 编码器速度环控制计算。 */
void PID_EncoderSpeed_Control(PID_EncoderSpeed_t* speed,
				      float actual_left,
				      float actual_right,
				      float* out_left,
				      float* out_right)
{
	if ((speed == 0) || (out_left == 0) || (out_right == 0))
	{
		return;
	}

	* out_left = PID_Calc(&speed->left, actual_left);
	* out_right = PID_Calc(&speed->right, actual_right);
}

#include "Control.h"

/* 1 开，0 关。 */
uint8_t pid_flag = 1U;
uint8_t pid_enabled = 1U;

/* 目标层：姿态与高度。 */
float Target_Pitch = 0.0f;
float Target_Roll = 0.0f;
float Target_Yaw = 0.0f;
float Target_Alt = 0.0f;

/* 陀螺零偏（原始 LSB）*/
static float gyro_bias_x = -32.1f;
static float gyro_bias_y = -3.1f;
static float gyro_bias_z = 0.0f;

/* 外环 PID */
PID_TypeDef pid_pitch;
PID_TypeDef pid_roll;
PID_TypeDef pid_yaw;
PID_TypeDef pid_alt;

/* 内环 PID */
PID_TypeDef pid_rate_pitch;
PID_TypeDef pid_rate_roll;
PID_TypeDef pid_rate_yaw;

/* Pitch/Roll 串级对象。 */
static PID_Cascade_t cascade_pitch;
static PID_Cascade_t cascade_roll;

/* 每个通道独立低通状态，避免旧静态接口导致通道串扰。 */
static LPF1_t gyro_pitch_lpf;
static LPF1_t gyro_roll_lpf;

/* 将陀螺仪原始值转换为角速度（deg/s）。 */
static float GyroRawToDps(short raw, float bias)
{
	/* 先扣除零偏，再按灵敏度缩放。 */
	return ((float)raw - bias) / GYRO_SENS_2000DPS;
}

/*
 * 电机加载函数
 */
void Motor_Test(void)
{
	/* 预留 */
}

/* PID 参数初始化。 */
void PID_Contorl_Init(void)
{
	/* 外环（角度）参数初始化。 */
	PID_Init(&pid_pitch, 3.0f, 0.02f, 0.03f);
	PID_Init(&pid_roll,  3.0f, 0.02f, 0.03f);
	PID_Init(&pid_yaw,   2.0f, 0.00f, 0.01f);
	PID_Init(&pid_alt,   0.0f, 0.00f, 0.00f);

	/* 内环（角速度）参数初始化。 */
	PID_Init(&pid_rate_pitch, 0.22f, 0.02f, 0.003f);
	PID_Init(&pid_rate_roll,  0.22f, 0.02f, 0.003f);
	PID_Init(&pid_rate_yaw,   0.15f, 0.00f, 0.000f);

	/* 控制周期：这里按 2ms (500Hz) 配置。 */
	PID_SetSampleTime(&pid_pitch, 0.002f);
	PID_SetSampleTime(&pid_roll, 0.002f);
	PID_SetSampleTime(&pid_rate_pitch, 0.002f);
	PID_SetSampleTime(&pid_rate_roll, 0.002f);

	/* 输出限幅与积分限幅。 */
	PID_SetOutputLimit(&pid_pitch, 2047.0f);
	PID_SetOutputLimit(&pid_roll, 2047.0f);

	PID_SetOutputLimit(&pid_rate_pitch, 2047.0f);
	PID_SetOutputLimit(&pid_rate_roll, 2047.0f);

	PID_SetIntegralLimit(&pid_pitch, 120.0f);
	PID_SetIntegralLimit(&pid_roll, 120.0f);

	PID_SetIntegralLimit(&pid_rate_pitch, 200.0f);
	PID_SetIntegralLimit(&pid_rate_roll, 200.0f);

	/* 微分低通，抑制角速度噪声放大。 */
	PID_SetDerivativeLPF(&pid_pitch, 0.25f);
	PID_SetDerivativeLPF(&pid_roll, 0.25f);

	PID_SetDerivativeLPF(&pid_rate_pitch, 0.35f);
	PID_SetDerivativeLPF(&pid_rate_roll, 0.35f);

	/* 死区与积分分离：减小微小抖动和大偏差积分堆积。 */
	PID_SetDeadband(&pid_pitch, 0.05f);
	PID_SetDeadband(&pid_roll, 0.05f);

	PID_SetIntegralSeparation(&pid_pitch, 8.0f);
	PID_SetIntegralSeparation(&pid_roll, 8.0f);

	/* 建立串级关系：外环角度 -> 内环角速度。 */
	PID_Cascade_Init(&cascade_pitch, &pid_pitch, &pid_rate_pitch);
	PID_Cascade_Init(&cascade_roll, &pid_roll, &pid_rate_roll);

	/* 角速度低通：每轴一个实例 */
	LPF1_Init(&gyro_pitch_lpf, 0.45f, 0.0f);
	LPF1_Init(&gyro_roll_lpf, 0.45f, 0.0f);
}

/* 设置陀螺仪零偏。 */
void Set_Gyro_Bias(float bias_x, float bias_y, float bias_z)
{
	gyro_bias_x = bias_x;
	gyro_bias_y = bias_y;
	gyro_bias_z = bias_z;
	(void)gyro_bias_z;
}

/*
 * Pitch 和 Roll 合并双环控制函数
 * 入口参数：
 * actual_pitch: Pitch 实际角度
 * actual_roll : Roll 实际角度
 */
void PID_Pitch_Roll_Combined(float actual_pitch, float actual_roll)
{
	/* 内环输出（最终用于电机混控）。 */
	float pitch_rate_out = 0.0f;
	float roll_rate_out = 0.0f;
	/* 陀螺角速度（deg/s）。 */
	float gyro_pitch_dps = 0.0f;
	float gyro_roll_dps = 0.0f;

	/* 控制开关保护。 */
	if ((pid_flag == 0U) || (pid_enabled == 0U))
	{
		return;
	}

	/* 外环目标角度来自上位控制（遥控/导航）。 */
	pid_pitch.Target = Target_Pitch;
	pid_roll.Target = Target_Roll;

	/* 角速度反馈：原始值 -> 去偏 -> deg/s。 */
	gyro_roll_dps = GyroRawToDps(gyrox, gyro_bias_x);
	gyro_pitch_dps = GyroRawToDps(gyroy, gyro_bias_y);

	/* 每个轴独立低通，抑制陀螺高频噪声。 */
	gyro_roll_dps = LPF1_Update(&gyro_roll_lpf, gyro_roll_dps);
	gyro_pitch_dps = LPF1_Update(&gyro_pitch_lpf, gyro_pitch_dps);

	/* 串级控制：外环角度 -> 内环角速度 -> 输出。 */
	pitch_rate_out = PID_Cascade_Calc(&cascade_pitch, actual_pitch, gyro_pitch_dps, 0.002f, 0.002f);
	roll_rate_out = PID_Cascade_Calc(&cascade_roll, actual_roll, gyro_roll_dps, 0.002f, 0.002f);

	/* 输出保护限幅，防止异常参数打满。 */
	pitch_rate_out = Limit_Output(pitch_rate_out, MOTOR_MIX_LIMIT);
	roll_rate_out = Limit_Output(roll_rate_out, MOTOR_MIX_LIMIT);

	/* 保留到 PID 对象，方便串口/示波器观察。 */
	pid_rate_pitch.output = pitch_rate_out;
	pid_rate_roll.output = roll_rate_out;

	/*
	 * 加载输出到电机：
	 * 这里按你的要求先预留统一入口，后续可替换为真实混控。
	 */
	Motor_Test();
}



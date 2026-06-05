#include "Control.h"

/* PID 任务节拍标志：由 2ms 定时节拍置位。 */
uint8_t pid_task_flag = 0U;

/* 目标层：姿态 */
float Target_Pitch = 0.0f;
float Target_Roll = 0.0f;
float Target_Yaw = 0.0f;

/* 陀螺零偏（原始 LSB）。 */
static float gyro_bias_x = -26.0f;
static float gyro_bias_y = -3.0f;
static float gyro_bias_z = 0.0f;

/* 陀螺自动零偏校准样本数：500Hz 下 1000 点约 2s。 */
static const uint16_t s_gyro_bias_calib_samples = 1000U;
/* 自动校准状态。 */
static uint16_t s_gyro_bias_sample_count = 0U;
static float s_gyro_bias_acc_x = 0.0f;
static float s_gyro_bias_acc_y = 0.0f;
static uint8_t s_gyro_bias_ready = 0U;

/* 控制回路周期：500Hz。 */
#define CONTROL_DT_S (0.002f)

/* 速度环 */
PID_EncoderSpeed_t speed_loop;
/* 外环 PID。 */
PID_TypeDef pid_pitch;
PID_TypeDef pid_roll;
PID_TypeDef pid_yaw;
/* 内环 PID。 */
PID_TypeDef pid_rate_pitch;
PID_TypeDef pid_rate_roll;
PID_TypeDef pid_rate_yaw;

/* Pitch/Roll 串级对象。 */
static PID_Cascade_t cascade_pitch;
static PID_Cascade_t cascade_roll;

/* 每个通道独立低通状态，避免通道串扰。 */
static LPF1_t gyro_pitch_lpf;
static LPF1_t gyro_roll_lpf;

/* 将陀螺仪原始值转换为角速度（deg/s）。 */
static float GyroRawToDps(short raw, float bias)
{
	return ((float)raw - bias) / GYRO_SENS_2000DPS;
}

/*
 * 陀螺零偏自动校准。
 * 返回 1 表示校准完成，0 表示仍在校准。
 */
static uint8_t GyroBiasAutoCalibStep(void)
{
	if (s_gyro_bias_ready != 0U)
	{
		return 1U;
	}

	s_gyro_bias_acc_x += (float)gyrox;
	s_gyro_bias_acc_y += (float)gyroy;
	s_gyro_bias_sample_count++;

	if (s_gyro_bias_sample_count >= s_gyro_bias_calib_samples)
	{
		Set_Gyro_Bias(s_gyro_bias_acc_x / (float)s_gyro_bias_sample_count,
					  s_gyro_bias_acc_y / (float)s_gyro_bias_sample_count,
					  0.0f);
		s_gyro_bias_ready = 1U;
		return 1U;
	}

	return 0U;
}

/* 电机加载函数预留。 */
void Motor_Test(void)
{
	/* 预留 */
}

/* 速度环初始化。 */
void PID_Speed_Init(void)
{
	PID_EncoderSpeed_Init(&speed_loop);

	/* 速度环周期与 Encoder_flag 保持一致（20ms） */
	PID_SetSampleTime(&speed_loop.left,  0.02f);
	PID_SetSampleTime(&speed_loop.right, 0.02f);
}

/* PID 参数初始化。 */
void PID_Contorl_Init(void)
{
	/* 外环（角度）初始化与配置。 */
	PID_Init(&pid_pitch);
	PID_Init(&pid_roll);

	/* 外环限幅初始化 */
	PID_Init_WithLimit(&pid_pitch, 700.0f, 400.0f);
	PID_Init_WithLimit(&pid_roll, 700.0f, 400.0f);

	/* 内环（角速度）初始化与配置。 */
	PID_Init(&pid_rate_pitch);
	PID_Init(&pid_rate_roll);

	/* 内环限幅初始化 */
	PID_Init_WithLimit(&pid_rate_pitch, 300.0f, MOTOR_MIX_LIMIT);
	PID_Init_WithLimit(&pid_rate_roll, 300.0f, MOTOR_MIX_LIMIT);

	/* 建立串级关系：外环角度 -> 内环角速度。 */
	PID_Cascade_Init(&cascade_pitch, &pid_pitch, &pid_rate_pitch);
	PID_Cascade_Init(&cascade_roll, &pid_roll, &pid_rate_roll);

	/* 角速度低通：每轴一个实例。 */
	LPF1_Init(&gyro_pitch_lpf, 0.45f, 0.0f);
	LPF1_Init(&gyro_roll_lpf, 0.45f, 0.0f);

	/* 重置自动校准状态。 */
	s_gyro_bias_sample_count = 0U;
	s_gyro_bias_acc_x = 0.0f;
	s_gyro_bias_acc_y = 0.0f;
	s_gyro_bias_ready = 0U;
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
 * Pitch 和 Roll 合并双环控制串级PID函数
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

	/* 只有节拍到来才执行一次 PID。 */
	if (pid_task_flag != 1U)
	{
		return;
	}
	pid_task_flag = 0U;

	/* 启动阶段先做零偏校准，避免积分带偏。 */
	if (GyroBiasAutoCalibStep() == 0U)
	{
		PID_Reset(&pid_pitch);
		PID_Reset(&pid_roll);
		PID_Reset(&pid_rate_pitch);
		PID_Reset(&pid_rate_roll);
		pid_rate_pitch.output = 0.0f;
		pid_rate_roll.output = 0.0f;
		Motor_Test();
		return;
	}

	/* 外环目标角度来自上位控制（遥控/导航）。 */
	PID_SetTarget(&pid_pitch, Target_Pitch);
	PID_SetTarget(&pid_roll, Target_Roll);

	/* 角速度反馈：原始值 -> 去偏 -> deg/s。 */
	gyro_roll_dps = GyroRawToDps(gyrox, gyro_bias_x);
	gyro_pitch_dps = GyroRawToDps(gyroy, gyro_bias_y);

	/* 每个轴独立低通，抑制陀螺高频噪声。 */
	gyro_roll_dps = LPF1_Update(&gyro_roll_lpf, gyro_roll_dps);
	gyro_pitch_dps = LPF1_Update(&gyro_pitch_lpf, gyro_pitch_dps);

	/* 串级控制统一调用 PID 库接口。 */
	pitch_rate_out = PID_Cascade_Calc(&cascade_pitch, actual_pitch, gyro_pitch_dps, CONTROL_DT_S, CONTROL_DT_S);
	roll_rate_out = PID_Cascade_Calc(&cascade_roll, actual_roll, gyro_roll_dps, CONTROL_DT_S, CONTROL_DT_S);

	/* 保留到 PID 对象，方便串口/示波器观察。 */
	pid_rate_pitch.output = pitch_rate_out;
	pid_rate_roll.output = roll_rate_out;

	/* 加载输出到电机：预留统一入口，后续混控。 */
	Motor_Test();
}

/* 速度环控制函数：由 main.c 在 Encoder_flag 节拍（20ms）调用一次。 */
void PID_Speed_Control(float actual_left, float actual_right)
{
	float out_left = 0.0f;
	float out_right = 0.0f;

	PID_EncoderSpeed_Control(&speed_loop, actual_left, actual_right, &out_left, &out_right);

	/* 加载输出到电机 */
	TB6612_SetSpeed((int16_t)out_left, (int16_t)out_right);
}

#ifndef __FILTER_H
#define __FILTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 滤波窗口最大长度。
 * 说明：为兼顾 RAM 占用与实时性，默认限制为 31。
 */
#define FILTER_MAX_WINDOW (31U)

/*
 * 一阶低通滤波器状态
 * 用途：平滑传感器高频噪声，常用于角速度、电流、ADC。
 */
typedef struct
{
	/* 平滑系数，范围[0,1]。越大越跟随输入，越小越平滑。 */
	float alpha;
	/* 上一次滤波输出。 */
	float output;
	/* 初始化标志。 */
	uint8_t inited;
} LPF1_t;

/*
 * 滑动平均滤波器状态
 * 用途：抑制随机噪声，代价是响应延迟增加。
 */
typedef struct
{
	float buffer[FILTER_MAX_WINDOW];
	float sum;
	uint8_t len;
	uint8_t index;
	uint8_t count;
} MovingAverage_t;

/*
 * 中值滤波器状态
 * 用途：对毛刺/尖峰噪声抑制效果好。
 */
typedef struct
{
	float buffer[FILTER_MAX_WINDOW];
	uint8_t len;
	uint8_t index;
	uint8_t count;
} MedianFilter_t;

/*
 * 一维卡尔曼滤波状态
 * 状态模型：x(k)=x(k-1)+w
 * 观测模型：z(k)=x(k)+v
 */
typedef struct
{
	/* 过程噪声协方差。 */
	float q;
	/* 观测噪声协方差。 */
	float r;
	/* 当前状态估计值。 */
	float x;
	/* 当前估计协方差。 */
	float p;
	/* 卡尔曼增益。 */
	float k;
	uint8_t inited;
} Kalman1D_t;

/*
 * 二状态姿态卡尔曼（角度 + 零偏）
 * 常见于 IMU 融合：输入加速度角度 + 陀螺角速度。
 */
typedef struct
{
	float q_angle;
	float q_bias;
	float r_measure;

	float angle;
	float bias;
	float rate;

	float p00;
	float p01;
	float p10;
	float p11;

	uint8_t inited;
} Kalman2D_Angle_t;

/*
 * 初始化一阶低通。
 * 用法示例：
 *   LPF1_t lpf;
 *   LPF1_Init(&lpf, 0.2f, 0.0f);
 */
void LPF1_Init(LPF1_t* f, float alpha, float init_output);

/*
 * 一阶低通更新。
 * 入参：input 为当前采样值。
 * 返回：滤波结果。
 */

 float LPF1_Update(LPF1_t* f, float input);

/*
 * 初始化滑动平均窗口。
 * len 建议 3~15；越大越平滑，延迟越大。
 */
void MovingAverage_Init(MovingAverage_t* f, uint8_t len);

/*
 * 滑动平均更新，返回窗口平均值。
 */
float MovingAverage_Update(MovingAverage_t* f, float input);

/*
 * 初始化中值滤波。
 * 若 len 是偶数，内部会自动减 1 变成奇数。
 */
void MedianFilter_Init(MedianFilter_t* f, uint8_t len);

/*
 * 中值滤波更新，返回窗口中值。
 */
float MedianFilter_Update(MedianFilter_t* f, float input);

/*
 * 初始化一维卡尔曼。
 * 调参经验：
 *   q 增大 -> 更快跟随变化
 *   r 增大 -> 更信任历史估计、抑制测量噪声
 */
void Kalman1D_Init(Kalman1D_t* f, float q, float r, float init_x, float init_p);

/*
 * 一维卡尔曼更新。
 * 返回：估计后的状态值。
 */
float Kalman1D_Update(Kalman1D_t* f, float measurement);

/*
 * 初始化二状态姿态卡尔曼。
 * init_angle 建议用上电静止姿态角。
 */
void Kalman2D_Angle_Init(Kalman2D_Angle_t* f,
						 float q_angle,
						 float q_bias,
						 float r_measure,
						 float init_angle,
						 float init_bias);

/*
 * 二状态姿态卡尔曼更新。
 * accel_angle: 加速度计解算角度(deg)
 * gyro_rate  : 陀螺角速度(deg/s)
 * dt         : 采样周期(s)
 * 返回：融合角度(deg)
 */
float Kalman2D_Angle_Update(Kalman2D_Angle_t* f, float accel_angle, float gyro_rate, float dt);

/* 限幅函数。 */
float Filter_Limit(float input, float max_abs);
/* 死区函数。 */
float Filter_Deadband(float input, float deadband);
/* 斜率限制函数。 */
float Filter_SlewRate(float input, float last_output, float max_step);
/* 互补滤波函数。 */
float Filter_Complementary(float angle_acc, float gyro_rate, float dt, float alpha, float last_angle);

/*
 * 说明：
 * 本库不再提供“单函数静态状态”的旧低通接口。
 * 推荐使用 LPF1_t + LPF1_Update 的实例化方式，
 * 每个通道各自维护滤波状态，避免通道间串扰。
 */

#ifdef __cplusplus
}
#endif


#endif /* FILTER_H */

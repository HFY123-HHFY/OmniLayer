#include "Filter.h"

#include <string.h>

/* 返回绝对值，供本文件内部使用。 */
static float filter_abs(float x)
{
	return (x >= 0.0f) ? x : -x;
}

/*
 * 简单冒泡排序。
 * 用于中值滤波小窗口排序，窗口很小时开销可接受。
 */
static void bubble_sort(float* data, uint8_t len)
{
	uint8_t i;
	uint8_t j;
	float tmp;

	for (i = 0U; i < len; ++i)
	{
		for (j = 0U; j + 1U < (uint8_t)(len - i); ++j)
		{
			if (data[j] > data[j + 1U])
			{
				tmp = data[j];
				data[j] = data[j + 1U];
				data[j + 1U] = tmp;
			}
		}
	}
}

/* 初始化一阶低通滤波器。 */
void LPF1_Init(LPF1_t* f, float alpha, float init_output)
{
	/* 空指针保护，避免上层未判空时写坏内存。 */
	if (f == 0)
	{
		return;
	}

	/* 把 alpha 限制在 [0,1]，避免数值失真。 */
	if (alpha < 0.0f)
	{
		alpha = 0.0f;
	}
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}

	f->alpha = alpha;
	f->output = init_output;
	f->inited = 1U;
}

/*
 * 一阶低通更新。
 * 公式：y(k)=alpha*x(k)+(1-alpha)*y(k-1)
 */
float LPF1_Update(LPF1_t* f, float input)
{
	/* 若对象无效，直接透传输入。 */
	if (f == 0)
	{
		return input;
	}

	/* 首次更新时把输出对齐到输入，减少上电突变。 */
	if (f->inited == 0U)
	{
		f->output = input;
		f->inited = 1U;
		return f->output;
	}

	/* 标准一阶 IIR 低通。 */
	f->output = (f->alpha * input) + ((1.0f - f->alpha) * f->output);
	return f->output;
}

/* 初始化滑动平均滤波器。 */
void MovingAverage_Init(MovingAverage_t* f, uint8_t len)
{
	/* 参数检查。 */
	if (f == 0)
	{
		return;
	}

	if (len == 0U)
	{
		len = 1U;
	}
	if (len > FILTER_MAX_WINDOW)
	{
		len = FILTER_MAX_WINDOW;
	}

	/* 清空历史样本与累加状态。 */
	memset(f->buffer, 0, sizeof(f->buffer));
	f->sum = 0.0f;
	f->len = len;
	f->index = 0U;
	f->count = 0U;
}

/*
 * 滑动平均更新。
 * 通过累加和减去旧值实现 O(1) 更新，避免每次遍历窗口。
 */
float MovingAverage_Update(MovingAverage_t* f, float input)
{
	/* 对象无效时直接返回输入。 */
	if (f == 0)
	{
		return input;
	}

	if (f->len == 0U)
	{
		f->len = 1U;
	}

	/* 从累加和里先减去即将被覆盖的旧样本。 */
	f->sum -= f->buffer[f->index];
	/* 写入新样本。 */
	f->buffer[f->index] = input;
	/* 再把新样本加入累加和。 */
	f->sum += input;

	/* 环形索引前进。 */
	f->index++;
	if (f->index >= f->len)
	{
		f->index = 0U;
	}

	/* 仅在窗口填满前递增有效样本数。 */
	if (f->count < f->len)
	{
		f->count++;
	}

	return f->sum / (float)f->count;
}

/* 初始化中值滤波器。 */
void MedianFilter_Init(MedianFilter_t* f, uint8_t len)
{
	/* 参数检查。 */
	if (f == 0)
	{
		return;
	}

	if (len == 0U)
	{
		len = 1U;
	}
	if (len > FILTER_MAX_WINDOW)
	{
		len = FILTER_MAX_WINDOW;
	}
	if ((len & 0x01U) == 0U)
	{
		len--;
	}

	/* 清空缓冲，重置游标。 */
	memset(f->buffer, 0, sizeof(f->buffer));
	f->len = len;
	f->index = 0U;
	f->count = 0U;
}

/*
 * 中值滤波更新。
 * 步骤：写入环形缓冲 -> 拷贝有效样本 -> 排序 -> 取中值。
 */
float MedianFilter_Update(MedianFilter_t* f, float input)
{
	/* 排序工作区：最多复制 FILTER_MAX_WINDOW 个样本。 */
	float sorted[FILTER_MAX_WINDOW];
	/* 当前有效窗口长度（启动阶段小于配置长度）。 */
	uint8_t valid_count;

	if (f == 0)
	{
		return input;
	}

	if (f->len == 0U)
	{
		f->len = 1U;
	}

	/* 写入当前样本到环形缓冲。 */
	f->buffer[f->index] = input;
	f->index++;
	if (f->index >= f->len)
	{
		f->index = 0U;
	}

	/* 维护有效计数。 */
	if (f->count < f->len)
	{
		f->count++;
	}

	valid_count = f->count;
	/* 只拷贝有效样本，避免对未写入区排序。 */
	memcpy(sorted, f->buffer, (size_t)valid_count * sizeof(float));
	/* 小窗口采用冒泡排序足够且易读。 */
	bubble_sort(sorted, valid_count);

	/* 返回中值（valid_count 保证为奇数或初始化阶段自然取中位）。 */
	return sorted[valid_count / 2U];
}

/* 初始化一维卡尔曼滤波器。 */
void Kalman1D_Init(Kalman1D_t* f, float q, float r, float init_x, float init_p)
{
	/* 参数检查。 */
	if (f == 0)
	{
		return;
	}

	/* 噪声参数做边界保护，避免除零与负协方差。 */
	f->q = (q >= 0.0f) ? q : 0.0f;
	f->r = (r > 0.0f) ? r : 1e-6f;
	/* 初始化状态与协方差。 */
	f->x = init_x;
	f->p = (init_p >= 0.0f) ? init_p : 0.0f;
	f->k = 0.0f;
	f->inited = 1U;
}

/*
 * 一维卡尔曼更新。
 * 预测：P = P + Q
 * 更新：K = P/(P+R), X = X + K*(Z-X), P = (1-K)*P
 */
float Kalman1D_Update(Kalman1D_t* f, float measurement)
{
	/* 对象无效时透传测量值。 */
	if (f == 0)
	{
		return measurement;
	}

	/* 懒初始化：首次调用时以测量值作为初值。 */
	if (f->inited == 0U)
	{
		Kalman1D_Init(f, 0.001f, 0.1f, measurement, 1.0f);
		return f->x;
	}

	/* 预测协方差：P(k|k-1)=P(k-1|k-1)+Q */
	f->p = f->p + f->q;
	/* 卡尔曼增益：K=P/(P+R) */
	f->k = f->p / (f->p + f->r);
	/* 状态更新：X=X+K*(Z-X) */
	f->x = f->x + f->k * (measurement - f->x);
	/* 协方差更新：P=(1-K)*P */
	f->p = (1.0f - f->k) * f->p;

	return f->x;
}

/*
 * 初始化二状态姿态卡尔曼。
 * 状态量：angle, bias
 */
void Kalman2D_Angle_Init(Kalman2D_Angle_t* f,
						 float q_angle,
						 float q_bias,
						 float r_measure,
						 float init_angle,
						 float init_bias)
{
	/* 参数检查。 */
	if (f == 0)
	{
		return;
	}

	f->q_angle = (q_angle >= 0.0f) ? q_angle : 0.0f;
	f->q_bias = (q_bias >= 0.0f) ? q_bias : 0.0f;
	f->r_measure = (r_measure > 0.0f) ? r_measure : 1e-6f;

	f->angle = init_angle;
	f->bias = init_bias;
	f->rate = 0.0f;

	/* 协方差初值：对角线给 1，表示初始不确定性适中。 */
	f->p00 = 1.0f;
	f->p01 = 0.0f;
	f->p10 = 0.0f;
	f->p11 = 1.0f;

	f->inited = 1U;
}

/*
 * 二状态姿态卡尔曼更新。
 * 这是嵌入式常见角度融合写法，适合 MPU6050 一类 IMU。
 */
float Kalman2D_Angle_Update(Kalman2D_Angle_t* f, float accel_angle, float gyro_rate, float dt)
{
	/* S: 创新协方差；k0/k1: 两个状态分量的增益；y: 创新。 */
	float s;
	float k0;
	float k1;
	float y;
	/* 使用临时量避免更新顺序导致协方差矩阵计算错误。 */
	float p00_tmp;
	float p01_tmp;
	float p10_tmp;
	float p11_tmp;

	if (f == 0)
	{
		return accel_angle;
	}

	if (f->inited == 0U)
	{
		Kalman2D_Angle_Init(f, 0.001f, 0.003f, 0.03f, accel_angle, 0.0f);
		return f->angle;
	}

	if (dt <= 0.0f)
	{
		return f->angle;
	}

	/* 预测步骤：先用去偏角速度积分得到先验角度。 */
	f->rate = gyro_rate - f->bias;
	f->angle += dt * f->rate;

	/* 预测协方差矩阵。 */
	f->p00 += dt * (dt * f->p11 - f->p01 - f->p10 + f->q_angle);
	f->p01 -= dt * f->p11;
	f->p10 -= dt * f->p11;
	f->p11 += f->q_bias * dt;

	/* 更新步骤：融合加速度计角度观测。 */
	s = f->p00 + f->r_measure;
	if (s <= 1e-9f)
	{
		s = 1e-9f;
	}

	k0 = f->p00 / s;
	k1 = f->p10 / s;
	y = accel_angle - f->angle;

	f->angle += k0 * y;
	f->bias += k1 * y;

	/* 先缓存旧协方差，再统一更新。 */
	p00_tmp = f->p00;
	p01_tmp = f->p01;
	p10_tmp = f->p10;
	p11_tmp = f->p11;

	f->p00 = p00_tmp - k0 * p00_tmp;
	f->p01 = p01_tmp - k0 * p01_tmp;
	f->p10 = p10_tmp - k1 * p00_tmp;
	f->p11 = p11_tmp - k1 * p01_tmp;

	return f->angle;
}

/* 限幅函数，限制输入幅值不超过 max_abs。 */
float Filter_Limit(float input, float max_abs)
{
	/* max_abs <= 0 时不限制，便于调试。 */
	if (max_abs <= 0.0f)
	{
		return input;
	}

	if (input > max_abs)
	{
		return max_abs;
	}
	if (input < -max_abs)
	{
		return -max_abs;
	}
	return input;
}

/* 死区函数，抑制小幅抖动。 */
float Filter_Deadband(float input, float deadband)
{
	/* 死区按绝对值处理，支持传入负值。 */
	deadband = filter_abs(deadband);
	if (filter_abs(input) <= deadband)
	{
		return 0.0f;
	}
	return input;
}

/* 斜率限制，约束每次更新的最大变化量。 */
float Filter_SlewRate(float input, float last_output, float max_step)
{
	/* diff: 当前步想要变化的幅度。 */
	float diff;

	max_step = filter_abs(max_step);
	diff = input - last_output;

	if (diff > max_step)
	{
		diff = max_step;
	}
	else if (diff < -max_step)
	{
		diff = -max_step;
	}

	return last_output + diff;
}

/*
 * 互补滤波。
 * 典型用法：
 *   angle = Filter_Complementary(acc_angle, gyro_rate, dt, 0.98f, angle);
 */
float Filter_Complementary(float angle_acc, float gyro_rate, float dt, float alpha, float last_angle)
{
	/* dt 非法则保持上次输出。 */
	if (dt <= 0.0f)
	{
		return last_angle;
	}

	if (alpha < 0.0f)
	{
		alpha = 0.0f;
	}
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}

	/* gyro 通道高通特性 + acc 通道低通特性。 */
	return (alpha * (last_angle + gyro_rate * dt)) + ((1.0f - alpha) * angle_acc);
}

#include <iostream>

using namespace std;

int main()
{
	cout << "Hello World!" << endl;
	// 线性回归示例数据
	double x[] = {1, 2, 3, 4, 5};
	double y[] = {2, 4, 5, 4, 5};
	int n = 5;

	double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
	for (int i = 0; i < n; ++i) {
		sum_x += x[i];
		sum_y += y[i];
		sum_xy += x[i] * y[i];
		sum_x2 += x[i] * x[i];
	}

	// 计算斜率和截距
	double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
	double intercept = (sum_y - slope * sum_x) / n;

	cout << "线性回归方程: y = " << slope << " * x + " << intercept << endl;
	return 0;
}
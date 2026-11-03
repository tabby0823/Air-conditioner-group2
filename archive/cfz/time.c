#include "function.h"

int main()
{
	float wholedistance = 0;
	float partdistance = 0;
	float speed = 0;
	int parttime = 0;
	int time = 0;
	int starttime[3];
	int endtime[3];
	printf("功能介绍：输入区间里程、时刻表上的区间起始和结束时间、到区间内任意位置的里程后，");
	printf("可以根据区间平均速度，计算由当前位置到区间内任意位置的大致运行时间。\n");
	printf("请输入时刻表上区间运行的起始时间（“第1天的08:08”表示为“1 8 8”）：\n");
	scanf_s("%2d %2d %2d", &starttime[0], &starttime[1], &starttime[2]);
	printf("请输入时刻表上区间运行的结束时间（“第10天的23:45”表示为“10 23 45”）：\n");
	scanf_s("%2d %2d %2d", &endtime[0], &endtime[1], &endtime[2]);
	printf("请输入时刻表上区间的里程（只需输入数字，单位为km）：\n");
	scanf_s("%5f", &wholedistance);
	printf("请输入到区间任意位置的里程（只需输入数字，单位为km）：\n");
	scanf_s("%5f", &partdistance);
	time = CalculateTime(starttime,3,endtime,3);
	parttime = CalculateResult(wholedistance,partdistance,time);
	PrintResult(parttime,starttime,3);

	return 0;
}
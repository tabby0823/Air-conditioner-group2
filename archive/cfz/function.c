#include "function.h"

int CalculateTime(int starttime[3], int a, int endtime[3], int b)
{
	int start = 0;
	int end = 0;
	start = 24 * 60 * starttime[0] + 60 * starttime[1] + starttime[2];
	end = 24 * 60 * endtime[0] + 60 * endtime[1] + endtime[2];
	int time = end - start;
	
	return time;
 }

int CalculateResult(float wholedistance, float partdistance, int time)
{
	float t = 0.0;
	t = (float)time;
	float speed = wholedistance / t;
	float p = partdistance / speed;
	int parttime = (int)p;

	return parttime;
}

void PrintResult(int parttime, int starttime[3], int c)
{
	int result1[2];
	int result2[3];
	int r = 0;
	//计算用时：
	result1[0] = parttime / 60;
	result1[1] = parttime % 60;
	//计算时刻：
	result2[0] = parttime / (24 * 60);
	r = parttime % (24 * 60);
	result2[1] = r / 60;
	result2[2] = r % 60;
	//打印用时：
	printf("到该位置用时约为：");
	printf("%2d", result1[0]);
	printf("小时");
	printf("%d", result1[1]);
	printf("分\n");
	//打印时刻：
	printf("到该位置的时间约为：");
	printf("第");
	printf("%d", result2[0] + starttime[0]);
	printf("天的");
	if ((0 <= result2[1] + starttime[1]) && (result2[1] + starttime[1] <= 10))
	{
		printf("0");
		printf("%d", result2[1] + starttime[1]);
	}
	else
	    printf("%d", result2[1] + starttime[1]);
	printf(":");
	if ((0 <= result2[2] + starttime[2]) && (result2[2] + starttime[2] <= 10))
	{
		printf("0");
		printf("%d", result2[2] + starttime[2]);
	}
	else
	    printf("%d", result2[2] + starttime[2]);
}
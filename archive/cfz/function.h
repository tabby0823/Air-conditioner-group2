#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

int CalculateTime(int starttime[3], int a,int endtime[3], int b);
int CalculateResult(float wholedistance, float partdistance, int time);
void PrintResult(int parttime, int starttime[3], int c);
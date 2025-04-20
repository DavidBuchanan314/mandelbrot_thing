#include <math.h>
#include <time.h>

#include "common.h"

double get_time_ms(void)
{
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return (double)spec.tv_sec * 1000.0f + ((double)spec.tv_nsec) / 1000000.0f;
}

/* distance between two points */
double dist(double x0, double y0, double x1, double y1)
{
	return sqrt((y1-y0)*(y1-y0)+(x1-x0)*(x1-x0));
}


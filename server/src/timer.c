#include "timer.h"
#include <time.h>
#include <stdint.h>
#include <windows.h>

static double perf_frequency;

void
os_time_init()
{
	LARGE_INTEGER li = { 0 };
	QueryPerformanceFrequency(&li);
	perf_frequency = (double)(li.QuadPart);
}

double
os_time_us()
{
	LARGE_INTEGER li = { 0 };
	QueryPerformanceCounter(&li);
	return ((double)(li.QuadPart) / perf_frequency) * 1000000.0;
}

double
timer_start(Timer* timer)
{
	timer->elapsed = 0.0f;
	timer->start_time = os_time_us();
	return timer->start_time;
}

double
timer_elapsed_us(Timer* timer, int reset)
{
	double result = os_time_us() - timer->start_time;
	if (reset) timer_start(timer);
	return result;
}

double
timer_elapsed_ms(Timer* timer, int reset)
{
	double result = (os_time_us() - timer->start_time) / 1000.0;
	if (reset) timer_start(timer);
	return result;
}

int
timer_has_elapsed_us(Timer* timer, double time_us, int reset_on_true)
{
	int res = (timer_elapsed_us(timer, 0) >= time_us);
	if (res && reset_on_true) timer_start(timer);
	return res;
}

int
timer_has_elapsed_ms(Timer* timer, double time_ms, int reset_on_true)
{
	int res = (timer_elapsed_ms(timer, 0) >= time_ms);
	if (res && reset_on_true) timer_start(timer);
	return res;
}
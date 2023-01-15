#pragma once
#include <common.h>

typedef struct {
	r64 start_time;
	r64 elapsed;
} Timer;

r64  timer_start(Timer* timer);
r64  timer_elapsed_us(Timer* timer, bool reset);
r64  timer_elapsed_ms(Timer* timer, bool reset);
bool timer_has_elapsed_us(Timer* timer, r64 time_us, bool reset_on_true);
bool timer_has_elapsed_ms(Timer* timer, r64 time_ms, bool reset_on_true);

void   os_time_init();
double os_time_us();
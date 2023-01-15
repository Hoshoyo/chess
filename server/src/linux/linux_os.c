#include <time.h>
#include <stdint.h>
#include "../os.h"

void
os_time_init()
{
    
}

double
os_time_us()
{
  struct timespec t_spec;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t_spec);
  uint64_t res = t_spec.tv_nsec + 1000000000 * t_spec.tv_sec;
  return (double) res / 1000.0;
}
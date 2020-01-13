#include <sys/time.h>

int gettimeofday(struct timeval *restrict tp, void *restrict tzp){
    tp->tv_sec = time(NULL);
    tp->tv_usec = time(NULL) * 1000 * 1000;
    return 0;
}

clock_t clk_count = 0;
time_t time_dat;
clock_t clock (void) {
    return (clk_count);
}

time_t time(time_t *var)
{
    return (int)xTaskGetTickCount()/1000;
}
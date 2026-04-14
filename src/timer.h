#ifndef _TIMER_H_
#define _TIMER_H_

int time_expired();

void start_timers();

double elapsed_time();

typedef enum type_timer {REAL, VIRTUAL} TIMER_TYPE;

#endif

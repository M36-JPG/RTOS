#ifndef HAL_RTC_H
#define HAL_RTC_H
#include <stdint.h>

bool hal_rtc_get_time(hal_time_t *t);
bool hal_rtc_set_time(const hal_time_t *t);

#endif
#ifndef __RTC_H__
#define __RTC_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t year;          // Year (0-99)
    uint8_t month;          // Month (1-12)
    uint8_t day;            // Day (1-31)
    uint8_t hour;           // Hour (0-23)
    uint8_t minute;         // Minute (0-59)
    uint8_t second;         // Second (0-59)
}rtc_date_t;

void rtc_init(void);
void rtc_set_date(rtc_date_t *date);            // Set the RTC date and time
void rtc_get_date(rtc_date_t *date);            // Get the RTC date and time
void rtc_set_timestamp(uint32_t timestamp);     // Set the RTC timestamp
void rtc_get_timestamp(uint32_t *timestamp);    // Get the RTC timestamp

#endif /* __RTC_H__ */

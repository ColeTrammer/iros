#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <kernel/hal/hw_device.h>
#include <kernel/hal/hw_timer.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/rtc.h>
#include <kernel/time/clock.h>
#include <kernel/util/init.h>

static inline uint8_t convert_from_bcd(uint8_t x) {
    return (x & 0xF) + ((x / 16) * 10);
}

static inline struct rtc_time read_rtc_time() {
    while (rtc_get(RTC_STATUS_A) & RTC_UPDATE_IN_PROGRESS)
        ;

    struct rtc_time current = { rtc_get(RTC_SECONDS), rtc_get(RTC_MINUTES), rtc_get(RTC_HOURS),  rtc_get(RTC_DAY_OF_MONTH),
                                rtc_get(RTC_MONTH),   rtc_get(RTC_YEAR),    rtc_get(RTC_CENTURY) };

    // Check for consistency if it really matters
#ifdef RTC_CHECK
    struct rtc_time last;
    do {
        memcpy(&last, &current, sizeof(struct rtc_time));

        current = (struct rtc_time) { rtc_get(RTC_SECONDS), rtc_get(RTC_MINUTES), rtc_get(RTC_HOURS),  rtc_get(RTC_DAY_OF_MONTH),
                                      rtc_get(RTC_MONTH),   rtc_get(RTC_YEAR),    rtc_get(RTC_CENTURY) };
    } while (!memcmp(&current, &last, sizeof(struct rtc_time)));
#endif /* RTC_CHECK */

    uint8_t status = rtc_get(RTC_STATUS_B);

    // convert if the format is bcd
    if (!(status & RTC_NOT_BCD)) {
        current.second = convert_from_bcd(current.second);
        current.minute = convert_from_bcd(current.minute);
        current.hour = convert_from_bcd(current.hour);
        current.day = convert_from_bcd(current.day);
        current.month = convert_from_bcd(current.month);
        current.year = convert_from_bcd(current.year);
        current.century = convert_from_bcd(current.century);
    }

    // handle 12 hour mode
    if (!(status & RTC_NOT_12_HOUR) && (current.hour & 0x80)) {
        current.hour = ((current.hour & 0x7F) + 12) & 24;
    }

    return current;
}

static struct hw_timer_ops rtc_ops = {};

static void detect_rtc(struct hw_device *parent) {
    struct rtc_time time = read_rtc_time();

    debug_log("RTC Seconds: [ %u ]\n", time.second);
    debug_log("RTC Minutes: [ %u ]\n", time.minute);
    debug_log("RTC Hours: [ %u ]\n", time.hour);
    debug_log("RTC Day of Month: [ %u ]\n", time.day);
    debug_log("RTC Month: [ %u ]\n", time.month);
    debug_log("RTC Year: [ %u ]\n", time.year + time.century * 100U);
    debug_log("RTC Century: [ %u ]\n", time.century);

    time_t seconds_since_epoch = time.second + 60L * time.minute + 3600L * time.hour + (time.day - 1) * 86400L;

    long current_year = time.century * 100L + time.year;
    for (long year = 1970; year <= current_year; year++) {
        for (long month = 1; month <= (year == current_year ? time.month - 1 : 12); month++) {
            long days = 0;
            switch (month) {
                case 1:  // Jan
                case 3:  // Mar
                case 5:  // May
                case 7:  // Jul
                case 8:  // Aug
                case 10: // Oct
                case 12: // Dec
                    days = 31;
                    break;
                case 4:  // Apr
                case 6:  // Jun
                case 9:  // Sep
                case 11: // Nov
                    days = 30;
                    break;
                case 2:
                    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) {
                        days = 29;
                    } else {
                        days = 28;
                    }
                    break;
                default:
                    assert(false);
            }

            seconds_since_epoch += days * 86400L;
        }
    }

    global_realtime_clock.time.tv_sec = seconds_since_epoch;
    // FIXME: seed a better RNG with this data
    srand(seconds_since_epoch);

    struct hw_timer *device = create_hw_timer("RTC", parent, hw_device_id_isa(), HW_TIMER_INTERVAL, &rtc_ops);
    device->hw_device.status = HW_STATUS_ACTIVE;
    register_hw_timer(device, (struct timespec) { .tv_sec = 1, .tv_nsec = 1000000 });
}

static struct isa_driver rtc_driver = {
    .name = "x86 RTC",
    .detect_devices = &detect_rtc,
};

static void init_rtc(void) {
    register_isa_driver(&rtc_driver);
}
INIT_FUNCTION(init_rtc, driver);

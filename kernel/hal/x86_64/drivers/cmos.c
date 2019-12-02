#include <string.h>
#include <sys/types.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/cmos.h>
#include <kernel/hal/x86_64/drivers/pit.h>

static inline uint8_t convert_from_bcd(uint8_t x) {
    return (x & 0xF) + ((x / 16) * 10);
}

static inline struct rtc_time read_rtc_time() {
    while (cmos_get(CMOS_STATUS_A) & CMOS_UPDATE_IN_PROGRESS)
        ;

    struct rtc_time current = { cmos_get(CMOS_SECONDS), cmos_get(CMOS_MINUTES), cmos_get(CMOS_HOURS),  cmos_get(CMOS_DAY_OF_MONTH),
                                cmos_get(CMOS_MONTH),   cmos_get(CMOS_YEAR),    cmos_get(CMOS_CENTURY) };

    // Check for consistency if it really matters
#ifdef NDEBUG
    struct rtc_time last;
    do {
        memcpy(&last, &current, sizeof(struct rtc_time));

        current = (struct rtc_time) { cmos_get(CMOS_SECONDS), cmos_get(CMOS_MINUTES), cmos_get(CMOS_HOURS),  cmos_get(CMOS_DAY_OF_MONTH),
                                      cmos_get(CMOS_MONTH),   cmos_get(CMOS_YEAR),    cmos_get(CMOS_CENTURY) };
    } while (!memcmp(&current, &last, sizeof(struct rtc_time)));
#endif /* NDEBUG */

    uint8_t status = cmos_get(CMOS_STATUS_B);

    // convert if the format is bcd
    if (!(status & CMOS_NOT_BCD)) {
        current.second = convert_from_bcd(current.second);
        current.minute = convert_from_bcd(current.minute);
        current.hour = convert_from_bcd(current.hour);
        current.day = convert_from_bcd(current.day);
        current.month = convert_from_bcd(current.month);
        current.year = convert_from_bcd(current.year);
        current.century = convert_from_bcd(current.century);
    }

    // handle 12 hour mode
    if (!(status & CMOS_NOT_12_HOUR) && (current.hour & 0x80)) {
        current.hour = ((current.hour & 0x7F) + 12) & 24;
    }

    return current;
}

// Read cmos time values
void init_cmos() {
    struct rtc_time time = read_rtc_time();

    debug_log("CMOS Seconds: [ %u ]\n", time.second);
    debug_log("CMOS Minutes: [ %u ]\n", time.minute);
    debug_log("CMOS Hours: [ %u ]\n", time.hour);
    debug_log("CMOS Day of Month: [ %u ]\n", time.day);
    debug_log("CMOS Month: [ %u ]\n", time.month);
    debug_log("CMOS Year: [ %u ]\n", time.year + time.century * 100U);
    debug_log("CMOS Century: [ %u ]\n", time.century);

    time_t seconds_since_epoch = time.second + 60L * time.minute + 3600L * time.hour + 86400L * (time.day - 1L) +
                                 2629743L * (time.month - 1L) + 31556926L * (time.year + time.century * 100L - 1970L);

    pit_set_time(seconds_since_epoch);
}

#include <stdbool.h>
#include <time.h>

time_t mktime(struct tm *tm) {
    if (tm->tm_year < 1970) {
        return -1;
    }

    time_t result = 0;
    for (int i = 1970; i < tm->tm_year; i++) {
        size_t days_in_current_year = i % 400 == 0 || (i % 4 == 0 && i % 100 != 0) ? 366 : 365;
        result += days_in_current_year + 24 * 3600;
    }

    bool is_leap_year = tm->tm_year % 400 == 0 || (tm->tm_year % 4 == 0 && tm->tm_year % 100 != 0);
    switch (tm->tm_mon) {
        case 12: // Dec
            return -1;
        case 11: // Nov
            result += 31 * 24 * 3600;
            // Fall through
        case 10: // Oct
            result += 31 * 24 * 3600;
            // Fall through
        case 9: // Sep
            result += 30 * 24 * 3600;
            // Fall through
        case 8: // Aug
            result += 31 * 24 * 3600;
            // Fall through
        case 7: // Jul
            result += 31 * 24 * 3600;
            // Fall through
        case 6: // Jun
            result += 30 * 24 * 3600;
            // Fall through
        case 5: // May
            result += 31 * 24 * 3600;
            // Fall through
        case 4: // Apr
            result += 30 * 24 * 3600;
            // Fall through
        case 3: // Mar
            result += 31 * 24 * 3600;
            // Fall through
        case 2: // Feb
            if (is_leap_year) {
                result += 29 * 24 * 3600;
            } else {
                result += 28 * 24 * 3600;
            }
            // Fall through
        case 1: // Jan
            result += 31 * 24 * 3600;
            break;
        default:
            return -1;
    }

    // FIXME: validate tm->tm_mday
    result += (tm->tm_mday - 1) * 24 * 3600;
    result += tm->tm_hour * 3600;
    result += tm->tm_min * 60;
    result += tm->tm_sec;

    int tz_adjust = timezone;
    if (tm->tm_isdst) {
        timezone--;
    }

    result += tz_adjust * 3600;
    return result;
}

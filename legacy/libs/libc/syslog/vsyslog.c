#include <bits/program_name.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "syslog_internal.h"

const char *sev_strs[8] = { "Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Info", "Debug" };

const char *facility_strs[17] = { "Kernel",  "User",    "Mail",    "News",    "UUCP",    "Daemon",  "Auth",    "Cron",   "LPR",
                                  "Local 0", "Local 1", "Local 2", "Local 3", "Local 4", "Local 5", "Local 6", "Local 7" };

void do_vsyslog(FILE *file, int prio, const char *format, va_list args, int saved_errno) {
    int facility = prio & ~7;
    if (!facility) {
        facility = log_facility;
    }

    int severity = prio & 7;

    if (!(LOG_MASK(severity) & log_mask)) {
        return;
    }

    if (file != stderr) {
        time_t time_now = time(NULL);
        char timestamp[56];
        ctime_r(&time_now, timestamp);
        fprintf(file, "[%s]: ", timestamp);

        const char *severity_string = sev_strs[severity];
        const char *facility_string = facility_strs[facility >> 3];
        fprintf(file, "%s (%s): ", severity_string, facility_string);
        if (log_options & LOG_PID) {
            fprintf(file, "PID %d: ", getpid());
        }
    }

    fprintf(file, "%s: ", log_identity ? log_identity : __program_name);
    errno = saved_errno;
    vfprintf(file, format, args);
    fputc('\n', file);
    fflush(file);
}

void vsyslog(int prio, const char *format, va_list args) {
    int saved_errno = errno;
    if (!log_file) {
        do_openlog();
    }

    if (log_options & LOG_PERROR) {
        do_vsyslog(stderr, prio, format, args, saved_errno);
    }
    do_vsyslog(log_file, prio, format, args, saved_errno);
}

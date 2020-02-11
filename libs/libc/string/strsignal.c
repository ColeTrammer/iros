#include <signal.h>

char *strsignal(int sig) {
    if (sig < 0 || sig > _NSIG) {
        sig = 0;
    }

    return (char *) sys_siglist[sig];
}

const char *const sys_siglist[_NSIG] = {
    "Invalid signal number",
    "TTY hang up",
    "Interrupted",
    "Quit",
    "Bus",
    "Trap",
    "Aborted",
    "Continued",
    "Floating point exeception",
    "Killed",
    "Read from tty",
    "Write to tty",
    "Illegal instruction",
    "Pipe error",
    "Alarm",
    "Terminated",
    "Segmentation fault",
    "Stopped",
    "Stopped by tty",
    "User 1",
    "User 2",
    "Poll",
    "Profile",
    "Invalid system call",
    "Urge",
    "Virtual alarm",
    "CPU exceeded time limit",
    "File size limit exceeded",
    "Child state change",
    "Invalid signal number",
    "Invalid signal number",
    "Invalid signal number",
};
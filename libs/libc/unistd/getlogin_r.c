#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int getlogin_r(char *buf, size_t bufsize) {
    struct stat st;

    // FIXME: this should tecnically use /dev/tty and not stdin
    if (fstat(STDIN_FILENO, &st) < 0) {
        errno = ENXIO;
        return -1;
    }

    struct passwd pwd;
    struct passwd *result;
    char pwd_buf[1024];
    if (!getpwuid_r(st.st_uid, &pwd, pwd_buf, sizeof(pwd_buf), &result)) {
        return -1;
    }

    size_t len = strlen(result->pw_name) + 1;
    if (len > bufsize) {
        errno = ERANGE;
        return -1;
    }

    strcpy(buf, result->pw_name);
    return 0;
}
#include <string.h>
#include <errno.h>

const char *sys_errlist[] = {
    "Errno start",
    "IO error",
    "Ran out of memory",
    "Invalid arguments",
    "File does not exist",
    "File cannot be executed",
    "Path is a directory",
    "Errno end"
};

int sys_nerr = EMAXERRNO;

char *strerror(int errnum) {
    if (errnum >= sys_nerr || errnum <= 0) {
        errno = EINVAL;
        return "Invalid Errno";
    }

    return (char*) sys_errlist[errnum];
}
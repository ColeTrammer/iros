#include <pwd.h>

struct passwd __static_passwd_buffer = { 0 };
char __static_passwd_string_buffer[1024] = { 0 };

struct passwd *getpwent(void) {
    struct passwd *ret;
    getpwent_r(&__static_passwd_buffer, __static_passwd_string_buffer, sizeof(__static_passwd_string_buffer), &ret);
    return ret;
}

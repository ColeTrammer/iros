#include <pwd.h>

extern struct passwd __static_passwd_buffer;
extern char __static_passwd_string_buffer[1024];

struct passwd *getpwuid(uid_t uid) {
    struct passwd *result;
    getpwuid_r(uid, &__static_passwd_buffer, __static_passwd_string_buffer, sizeof(__static_passwd_string_buffer), &result);
    return result;
}

#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char *user = "root";
    if (argc == 2) {
        user = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s <user>\n", argv[0]);
        return 2;
    }

    struct passwd *pw = getpwnam(user);
    if (!pw) {
        perror("getpwnam");
        return 1;
    }

    if (setuid(pw->pw_uid)) {
        perror("setuid");
        return 1;
    }

    if (setgid(pw->pw_gid)) {
        perror("setgid");
        return 1;
    }

    execl(pw->pw_shell, pw->pw_shell, NULL);
    perror("execl");
    return 1;
}

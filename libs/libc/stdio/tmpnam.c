#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char tmp_name_buffer[L_tmpnam] = { 0 };
static bool is_tmp_name_buffer_initialzied = false;

char *tmpnam(char *s) {
    if (!is_tmp_name_buffer_initialzied) {
        pid_t pid = getpid();

        strcpy(tmp_name_buffer, P_tmpdir);
        strcat(tmp_name_buffer, "/");
        for (size_t i = strlen(P_tmpdir) + 1; i < strlen(P_tmpdir) + 1 + 10; i++) {
            tmp_name_buffer[i] = ((pid + i) % 26) + 'a';
        }

        is_tmp_name_buffer_initialzied = true;
    }

    for (size_t i = strlen(P_tmpdir) + 1; i < strlen(P_tmpdir) + 1 + 10; i++) {
        tmp_name_buffer[i] = (((tmp_name_buffer[i] + 1) - 'a') % 26) + 'a';
    }

    if (s) {
        memcpy(s, tmp_name_buffer, L_tmpnam);
    }

    return s ? s : tmp_name_buffer;
}

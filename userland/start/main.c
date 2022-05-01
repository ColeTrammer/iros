#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <mntent.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

void spawn_process(char **argv, uid_t uid, gid_t gid, bool redirect, bool synchronize) {
    int fd[2];
    if (synchronize) {
        if (pipe(fd)) {
            perror("pipe");
            exit(1);
        }
    }

    int pid = fork();
    if (pid == 0) {
        if (uid != 0) {
            struct passwd *pwd = getpwuid(uid);
            assert(pwd);

            if (initgroups(pwd->pw_name, pwd->pw_gid)) {
                perror("initgroups");
                exit(1);
            }

            if (setgid(gid)) {
                perror("setgid");
                exit(1);
            }

            if (setuid(uid)) {
                perror("setuid");
                exit(1);
            }

            if (chdir(getpwuid(uid)->pw_dir)) {
                perror("chdir");
                exit(1);
            }
        }
        setpgid(0, 0);
        if (redirect) {
            int dev_null = open("/dev/null", O_RDWR, 0);
            dup2(dev_null, 0);
            dup2(dev_null, 1);
            dup2(dev_null, 2);
            close(dev_null);
        }
        char env_buffer[255];
        if (synchronize) {
            snprintf(env_buffer, sizeof(env_buffer) - 1, "__SYNCHRONIZE=%d", fd[1]);
            putenv(env_buffer);
            close(fd[0]);
        }
        if (execvp(argv[0], argv)) {
            perror("execvp");
            _exit(127);
        }

        _exit(1);
    } else if (pid == -1) {
        perror("fork");
        _exit(1);
    }

    setpgid(pid, pid);

    if (synchronize) {
        char c;
        close(fd[1]);
        assert(read(fd[0], &c, 1) == 1);
        close(fd[0]);
    }
}

int main(int argc, char **argv) {
    putenv("PATH=/bin:/usr/bin:/initrd");
    int serial_debug = open("/dev/serial0", O_RDWR);
    if (serial_debug == -1) {
        serial_debug = open("/dev/null", O_RDWR);
        assert(serial_debug != -1);
    }
    dup2(serial_debug, STDOUT_FILENO);
    dup2(serial_debug, STDERR_FILENO);

    bool use_graphics = true;
    int opt;
    while ((opt = getopt(argc, argv, ":gv")) != -1) {
        switch (opt) {
            case 'g':
                use_graphics = true;
                break;
            case 'v':
                use_graphics = false;
                break;
            case ':':
            case '?':
                fprintf(stderr, "Usage: %s [-g|-v]\n", *argv);
                break;
        }
    }

    // Mounting file systmes should be done via a separate program.
    FILE *mounts = setmntent("/etc/fstab", "r");
    if (!mounts) {
        perror("start");
        return 1;
    }

    struct mntent *mntent;
    while ((mntent = getmntent(mounts))) {
        if (strcmp(mntent->mnt_fsname, "none") == 0) {
            mntent->mnt_fsname = "";
        }
        mkdir(mntent->mnt_dir, 0777);
        if (mount(mntent->mnt_fsname, mntent->mnt_dir, mntent->mnt_type, 0, mntent->mnt_opts)) {
            fprintf(stderr, "Failed to mount `%s' on `%s': %s\n", mntent->mnt_fsname, mntent->mnt_dir, strerror(errno));
        }
    }

    endmntent(mounts);

    // Device setup should be done via a separate program.
    if (symlink("/proc/self/fd", "/dev/fd") || symlink("/proc/self/fd/0", "/dev/stdin") || symlink("/proc/self/fd/1", "/dev/stdout") ||
        symlink("/proc/self/fd/2", "/dev/stderr") || symlink("urandom", "/dev/random") || chown("/dev/fb0", 0, 14) ||
        chmod("dev/fb0", 0660)) {
        perror("start");
        return 1;
    }

    char *dhcp_client_args[] = { "/bin/dhcp_client", NULL };
    spawn_process(dhcp_client_args, 0, 0, false, false);

    char *nslookup_args[] = { "/bin/dns", "-d", NULL };
    spawn_process(nslookup_args, 12, 12, false, false);

    char *clipboard_server_args[] = { "/bin/clipboard_server", NULL };
    spawn_process(clipboard_server_args, 11, 11, false, false);

    if (!use_graphics) {
        char *terminal_args[] = { "/bin/terminal", "-v", NULL };
        spawn_process(terminal_args, 100, 100, false, false);
    } else {
        char *window_server_args[] = { "/bin/window_server", NULL };
        spawn_process(window_server_args, 10, 10, false, true);

        char *taskbar_args[] = { "/bin/taskbar", NULL };
        spawn_process(taskbar_args, 100, 100, false, false);

        char *window_server_test_args[] = { "/bin/terminal", NULL };
        spawn_process(window_server_test_args, 100, 100, false, false);
    }

    for (;;) {
        waitpid(-1, NULL, 0);
    }

    assert(false);
    return 0;
}

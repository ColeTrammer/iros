#include <app/event_loop.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/os_2.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>

#include "profile.h"

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [-o output] [-v file] <args...>\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    const char* output_path = "profile.data";
    const char* to_view = nullptr;

    int opt;
    while ((opt = getopt(argc, argv, ":o:v:")) != -1) {
        switch (opt) {
            case 'o':
                output_path = optarg;
                break;
            case 'v':
                to_view = optarg;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (to_view) {
        view_profile(to_view);
        return 0;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (enable_profiling(getpid())) {
            fprintf(stderr, "profile: start_profiling: %m\n");
            _exit(1);
        }
        execvp(argv[optind], argv + optind);
        fprintf(stderr, "profile: execv: %m\n");
        _exit(127);
    } else if (pid < 0) {
        fprintf(stderr, "profile: fork: %m\n");
        return 1;
    }

    void* buffer = mmap(0, PROFILE_BUFFER_MAX, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (buffer == MAP_FAILED) {
        fprintf(stderr, "profile: mmap: %m\n");
        return 1;
    }

    App::EventLoop loop;
    App::EventLoop::register_signal_handler(SIGCHLD, [&] {
        char name_buffer[PATH_MAX];
        memset(name_buffer, 0, sizeof(name_buffer));
        ssize_t ret = readlink(String::format("/proc/%d/exe", pid).string(), name_buffer, sizeof(name_buffer) - 1);
        if (ret < 0) {
            fprintf(stderr, "profile: readlink: %m\n");
            exit(1);
        }
        size_t name_size = ALIGN_UP(ret + 1, sizeof(size_t));

        ret = read_profile(pid, buffer, PROFILE_BUFFER_MAX);
        if (ret < 0) {
            fprintf(stderr, "profile: read_profile: %m\n");
            exit(1);
        }

        FILE* output_file = fopen(output_path, "w");
        if (!output_file) {
            fprintf(stderr, "profile: fopen: %m\n");
            exit(1);
        }

        if (fwrite(&name_size, sizeof(size_t), 1, output_file) != 1) {
            fprintf(stderr, "profile: fwrite: %m\n");
            exit(1);
        }

        if (fwrite(name_buffer, 1, name_size, output_file) != name_size) {
            fprintf(stderr, "profile: fprintf: %m\n");
            exit(1);
        }

        if (fwrite(buffer, 1, ret, output_file) != static_cast<size_t>(ret)) {
            fprintf(stderr, "profile: fwrite: %m\n");
            exit(1);
        }

        if (fclose(output_file) == EOF) {
            fprintf(stderr, "profile: fclose: %m\n");
            exit(1);
        }

        pid_t wait_result = waitpid(pid, NULL, WNOHANG);
        if (wait_result < 0) {
            fprintf(stderr, "profile: waitpid: %m\n");
        } else if (wait_result == 0) {
            return;
        }

        assert(wait_result == pid);
        view_profile(output_path);
    });

    loop.enter();
    munmap(buffer, PROFILE_BUFFER_MAX);
    return 0;
}

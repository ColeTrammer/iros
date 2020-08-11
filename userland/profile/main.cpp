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

enum Flags {
    Disable,
    Wait,
};

static void build_and_view_profile(pid_t pid, const char* output_path, int flags) {
    void* buffer = mmap(0, PROFILE_BUFFER_MAX, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (buffer == MAP_FAILED) {
        fprintf(stderr, "profile: mmap: %m\n");
        exit(1);
    }

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

    if (flags & Flags::Disable) {
        if (disable_profiling(pid)) {
            fprintf(stderr, "profile: disable_profiling: %m\n");
            exit(1);
        }
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

    printf("Wrote profile of `%s' to `%s'\n", name_buffer, output_path);

    if (flags & Flags::Wait) {
        pid_t wait_result = waitpid(pid, NULL, WNOHANG);
        if (wait_result < 0) {
            fprintf(stderr, "profile: waitpid: %m\n");
            exit(1);
        } else if (wait_result == 0) {
            return;
        }
        assert(wait_result == pid);
    }

    view_profile(output_path);
    munmap(buffer, PROFILE_BUFFER_MAX);
}

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [-o output] [-p pid]] [-v file] <args...>\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    const char* output_path = "profile.data";
    const char* to_view = nullptr;
    pid_t pid_to_profile = 0;

    int opt;
    while ((opt = getopt(argc, argv, ":o:p:v:")) != -1) {
        switch (opt) {
            case 'o':
                output_path = optarg;
                break;
            case 'p': {
                char* end;
                pid_to_profile = strtol(optarg, &end, 10);
                if (!end || *end || pid_to_profile <= 0) {
                    print_usage_and_exit(*argv);
                }
                break;
            }
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

    if (pid_to_profile) {
        if (enable_profiling(pid_to_profile)) {
            fprintf(stderr, "profile: enable_profiling: %m\n");
            return 1;
        }

        printf("Press any key to stop profiling...");
        fflush(stdout);
        fgetc(stdin);

        build_and_view_profile(pid_to_profile, output_path, Flags::Disable);
        return 0;
    }

    if (optind == argc) {
        print_usage_and_exit(*argv);
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

    App::EventLoop loop;
    App::EventLoop::register_signal_handler(SIGCHLD, [&] {
        build_and_view_profile(pid, output_path, Flags::Wait);
        loop.set_should_exit(true);
    });

    loop.enter();
    return 0;
}

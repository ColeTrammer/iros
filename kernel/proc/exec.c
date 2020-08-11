#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <kernel/fs/vfs.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/process.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>

static int execve_helper(char **path, char *buffer, size_t buffer_length, struct file **file, char ***prepend_argv,
                         size_t *prepend_argv_length, int *depth, char **argv) {
    if (*file) {
        fs_close(*file);
    }

    int ret = 0;
    *file = fs_openat(get_current_task()->process->cwd, *path, O_RDONLY, 0, &ret);
    if (ret < 0) {
        return ret;
    }

    ssize_t nread = fs_read(*file, buffer, buffer_length);
    if (nread < 0) {
        fs_close(*file);
        return (int) nread;
    } else if (nread == 0) {
        fs_close(*file);
        return -ENOEXEC;
    }

    if (!elf64_is_valid(buffer)) {
        if (memcmp(buffer, "#!", 2) == 0) {
            debug_log("Encoutered #!\n");
            bool first = *prepend_argv_length == 0;

            char *path_save = NULL;
            if (first) {
                path_save = strdup(*path);
            }
            size_t path_len = strcspn(buffer + 2, " \n");
            char restore = buffer[2 + path_len];
            buffer[2 + path_len] = '\0';
            free(*path);
            *path = strdup(buffer + 2);
            debug_log("#!: [ %s ]\n", *path);
            buffer[2 + path_len] = restore;
            bool has_extra_arg = false;
            size_t extra_arg_start = 0;

            size_t i;
            for (i = 0; buffer[2 + path_len + i] != '\n' && buffer[2 + path_len + i] != '\0'; i++) {
                if (!extra_arg_start && !isspace(buffer[2 + path_len + i])) {
                    has_extra_arg = true;
                    extra_arg_start = 2 + path_len + i;
                    break;
                }
            }

            (*prepend_argv_length) += first;

            if (has_extra_arg) {
                (*prepend_argv_length) += 2;
            } else {
                (*prepend_argv_length)++;
            }

            *prepend_argv = realloc(*prepend_argv, *prepend_argv_length * sizeof(char *));
            if (has_extra_arg) {
                buffer[2 + path_len] = '\0';
                *prepend_argv[*prepend_argv_length - 3] = strdup(buffer + 2);
                buffer[2 + path_len] = restore;
                buffer[2 + path_len + i] = '\0';
                *prepend_argv[*prepend_argv_length - 2] = strdup(buffer + extra_arg_start);
            } else {
                buffer[2 + path_len] = '\0';
                *prepend_argv[*prepend_argv_length - 2] = strdup(buffer + 2);
                buffer[2 + path_len] = restore;
            }

            (*prepend_argv)[*prepend_argv_length - 1] = NULL;

            if (first) {
                argv[0] = path_save;
            }

            (*depth)++;
            return execve_helper(path, buffer, buffer_length, file, prepend_argv, prepend_argv_length, depth, argv);
        }

        fs_close(*file);
        return -ENOEXEC;
    }

    return 0;
}

int proc_execve(char *path, char **argv, char **envp) {
    struct task *current = get_current_task();
    struct process *process = current->process;

    debug_log("Exec Task: [ %d, %s ]\n", process->pid, path);

    path = strdup(path);
    char temp_buffer[512];
    size_t temp_buffer_length = sizeof(temp_buffer);
    struct file *file = NULL;
    char **prepend_argv = NULL;
    size_t prepend_argv_length = 0;
    int depth = 0;
    int error = execve_helper(&path, temp_buffer, temp_buffer_length, &file, &prepend_argv, &prepend_argv_length, &depth, argv);
    free(path);
    if (error) {
        for (size_t i = 0; prepend_argv && i < prepend_argv_length; i++) {
            free(prepend_argv[i]);
        }
        free(prepend_argv);
        return error;
    }

    mutex_lock(&process->lock);
    // Some other thread called exit/execve before this one told them to exit.
    if (current->should_exit) {
        mutex_unlock(&process->lock);
        fs_close(file);
        for (size_t i = 0; prepend_argv && i < prepend_argv_length; i++) {
            free(prepend_argv[i]);
        }
        free(prepend_argv);
        return 0;
    }

    // Tell other threads to die.
    exit_process(process, current);
    process->in_execve = true;

    for (;;) {
        // Check if all the other threads have been removed.
        if (process->main_tid == current->tid && current->process_next == NULL && current->process_prev == NULL) {
            break;
        }

        // FIXME: this is clearly a racy
        mutex_unlock(&process->lock);
        wait_on(&process->one_task_left_queue);
        mutex_lock(&process->lock);
    }

    // Clear the profile buffer. This is means that code that sets up profiling need not worry about collecting data
    // before the execve() occurs. However, this would cause issues when trying to profile a process like /bin/sh, who
    // may have a legitimate reason to call execve().
    process->profile_buffer_size = 0;
    mutex_unlock(&process->lock);

    char *program_name = prepend_argv != NULL ? argv[0] : strdup(argv[0]);

    if (process->args_context) {
        free_program_args(process->args_context);
        process->args_context = NULL;
    }
    proc_clone_program_args(process, prepend_argv, argv, envp);
    soft_remove_paging_structure(process->process_memory);
    proc_reset_for_execve(process);

    assert(file->tnode);
    struct tnode *tnode = bump_tnode(file->tnode);
    struct inode *inode = fs_file_inode(file);
    process->exe = tnode;
    process->name = program_name;

    if (depth == 0 && (inode->mode & S_ISUID)) {
        process->euid = inode->uid;
    }

    if (depth == 0 && (inode->mode & S_ISGID)) {
        process->egid = inode->gid;
    }

    // Close CLOEXEC files.
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (process->files[i].file && ((process->files[i].fd_flags & FD_CLOEXEC) || (process->files[i].file->flags & FS_DIR))) {
            fs_close(process->files[i].file);
            process->files[i].file = NULL;
            continue;
        }
    }
    assert(process->main_tid == current->tid);
    assert(process->task_list == current);

    // Reset signals that have actual handlers.
    for (int i = 0; i < _NSIG; i++) {
        if ((uintptr_t) process->sig_state[i].sa_handler > (uintptr_t) SIG_IGN) {
            process->sig_state[i].sa_handler = SIG_DFL;
            process->sig_state[i].sa_flags = 0;
        }
    }

    size_t length = fs_file_size(file);
    char *buffer = (char *) fs_mmap(NULL, length, PROT_READ, MAP_SHARED, file, 0);
    // FIXME: this assert is very dangerous, but we can't return an error condition since
    //        we've already destroyed the old process's address space.
    assert(buffer != MAP_FAILED);

    struct initial_process_info info = { 0 };
    info.main_tid = current->tid;
    info.isatty_mask = 0;
    for (int i = 0; i <= 3; i++) {
        struct file *file = process->files[i].file;
        if (file && !fs_ioctl(file, TISATTY, NULL)) {
            info.isatty_mask |= (1 << i);
        }
    }

    assert(elf64_is_valid(buffer));
    task_set_ip(current->arch_task.user_task_state, elf64_load_program(buffer, length, file, &info));
    elf64_map_heap(current, &info);

    fs_close(file);
    unmap_range((uintptr_t) buffer, length);

    uintptr_t stack_end = proc_allocate_user_stack(process, &info);
    task_set_sp(current->arch_task.user_task_state, map_program_args(stack_end, process->args_context, &info, current));

    for (size_t i = 0; prepend_argv && i < prepend_argv_length; i++) {
        free(prepend_argv[i]);
    }
    free(prepend_argv);

    disable_interrupts();
    process->in_execve = false;

    // Clear this now while interrupts are disabled to ensure the initial value will be 0 (as POSIX says).
    memset(&current->task_clock->time, 0, sizeof(current->task_clock->time));

    // The FPU state must be reset here so that scheduler preemption won't over write the new value.
    memcpy(current->fpu.aligned_state, get_idle_task()->fpu.aligned_state, FPU_IMAGE_SIZE);
    return 0;
}

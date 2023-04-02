#pragma once

namespace iris {
enum class SystemCall {
    debug_print = 0,
    shutdown = 1,
    exit_task = 2,
    create_task = 3,
    load_executable = 4,
    start_task = 5,
    allocate_memory = 6,
    open = 7,
    read = 8,
    write = 9,
    close = 10,
    start_task_and_block = 11,
    set_userspace_thread_pointer = 12,
    set_userspace_stack_pointer = 13,
    set_userspace_instruction_pointer = 14,
    set_userspace_argument1 = 15,
    lseek = 16,
    set_task_arguments = 17,
};
}

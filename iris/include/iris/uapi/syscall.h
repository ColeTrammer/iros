#pragma once

namespace iris {
enum class SystemCall {
    debug_print = 0,
    shutdown = 1,
    exit_task = 2,
    create_task = 3,
    load_executable = 4,
    start_task = 5,
};
}

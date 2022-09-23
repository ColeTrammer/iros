#include <di/prelude.h>
#include <iris/core/log.h>
#include <limine.h>

[[noreturn]] static void done() {
    for (;;) {
        asm volatile("mov $52, %eax\n"
                     "cli\n"
                     "hlt\n");
    }
    di::unreachable();
}

extern "C" {
void iris_main() {
    iris::debug_log("Hello, World\n"_sv);
    done();
}
}
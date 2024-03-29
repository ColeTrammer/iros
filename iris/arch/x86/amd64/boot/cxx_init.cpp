#include <di/types/prelude.h>
#include <iris/boot/cxx_init.h>

extern "C" {
typedef void (*init_function_t)(void);

extern init_function_t __iris_init_array_start[];
extern init_function_t __iris_init_array_end[];
}

namespace iris::arch {
void cxx_init() {
    iptr init_size = __iris_init_array_end - __iris_init_array_start;
    for (iptr i = 0; i < init_size; i++) {
        (*__iris_init_array_start[i])();
    }
}
}

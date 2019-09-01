#include <kernel/hal/timer.h>
#include <kernel/hal/x86_64/drivers/pit.h>

void register_callback(void (*callback)(void), unsigned int ms) {
    pit_set_rate(ms);
    pit_register_callback(callback);
}
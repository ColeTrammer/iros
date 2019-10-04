#ifndef _KERNEL_HAL_X86_64_DRIVERS_PIT_H
#define _KERNEL_HAL_X86_64_DRIVERS_PIT_H 1

#include <sys/time.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/arch/x86_64/proc/process.h>

#define PIT_IRQ_LINE 0

#define PIT_CHANNEL_0 0x40
#define PIT_CHANNEL_1 0x41
#define PIT_CHANNEL_2 0x42
#define PIT_COMMAND 0x43

#define PIT_ACCESS_LO 0b01
#define PIT_ACCESS_HI 0b10
#define PIT_ACCESS_LOHI 0b11

#define PIT_MODE_INTERRUPT 0
#define PIT_MODE_ONE_SHOT 1
#define PIT_MODE_RATE_GENERATOR 2
#define PIT_MODE_SQUARE_WAVE 3
#define PIT_MODE_SOFTWARE_STROBE 4
#define PIT_MODE_HARDWARE_STROBE 5

#define PIT_CHANNEL(n) ((n << 6) & 0b11000000)
#define PIT_ACCESS(n) ((n << 4) & 0b00110000)
#define PIT_MODE(n) ((n << 1) & 0b00001110)

#define PIT_SET_MODE(ch, ac, m) outb(PIT_COMMAND, PIT_CHANNEL(ch) | PIT_ACCESS(ac) | PIT_MODE(m))

#define PIT_BASE_RATE (1.193182 * 1000.0) // Hz
#define PIT_GET_DIVISOR(ms) ((int) (((ms) * PIT_BASE_RATE) + 0.5))

void handle_pit_interrupt_entry();
void handle_pit_interrupt(struct process_state *process_state);

void init_pit();

void pit_set_rate(unsigned int rate);
void pit_set_sched_callback(void (*callback)(struct process_state*), unsigned int ms);
void pit_register_callback(void (*callback)(), unsigned int ms);
time_t pit_get_time();

#endif /* _KERNEL_HAL_X86_64_DRIVERS_PIT_H */
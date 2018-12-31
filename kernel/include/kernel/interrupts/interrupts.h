#ifndef _KERNEL_INTERRUPTS_INTERRUPTS_H
#define _KERNEL_INTERRUPTS_INTERRUPTS_H 1

void init_interrupts();

void handle_double_fault();
void handle_page_fault(uint64_t address);

#endif /* _KERNEL_INTERRUPTS_INTERRUPTS_H */
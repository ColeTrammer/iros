#ifndef _KERNEL_HAL_IRQS_H
#define _KERNEL_HAL_IRQS_H

#include <stdbool.h>

void init_irqs();

bool is_irq_registered(unsigned int irq);
void register_irq_handler(void *handler, unsigned int irq, bool is_user, bool needs_separate_stack);
void unregister_irq_handler(unsigned int irq);

#endif /* _KERNEL_HAL_IRQS_H */

#ifndef _HAL_IRQS_H
#define _HAL_IRQS_H

#include <stdbool.h>

void init_irqs();

void register_irq_handler(void *handler, unsigned int irq, bool is_user);
void unregister_irq_handler(unsigned int irq);

#endif
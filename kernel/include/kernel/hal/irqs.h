#ifndef _HAL_IRQS_H
#define _HAL_IRQS_H

void init_irqs();

void register_irq_handler(void *handler, unsigned int irq);
void unregister_irq_handler(unsigned int irq);

#endif
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>

static struct processor *processor_list;

struct processor *create_processor(uint8_t id) {
    struct processor *processor = malloc(sizeof(struct processor));
    processor->next = NULL;
    processor->id = id;
    processor->enabled = false;
    return processor;
}

struct processor *get_processor_list(void) {
    return processor_list;
}

void add_processor(struct processor *processor) {
    // NOTE: this should be called during boot when there is no potential for alternate CPUs to be running.
    processor->next = processor_list;
    processor_list = processor;

    debug_log("Processor detected: [ %u ]\n", processor->id);
}

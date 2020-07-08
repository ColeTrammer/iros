#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>

static struct processor *processor_list;
static int num_processors;

struct processor *create_processor() {
    struct processor *processor = malloc(sizeof(struct processor));
    processor->self = processor;
    processor->next = NULL;
    processor->kernel_stack = NULL;
    processor->id = num_processors++;
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

    if (processor->id == 0) {
        init_bsp(processor);
    }
}

int processor_count(void) {
    return num_processors;
}

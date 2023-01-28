#include <kernel/net/umessage.h>
#include <kernel/util/init.h>

static struct umessage_category umessage_input = {
    .category = UMESSAGE_INPUT,
    .request_type_count = UMESSAGE_INPUT_NUM_REQUESTS,
    .name = "UMessage Input",
};

static void init_umessage_input(void) {
    net_register_umessage_category(&umessage_input);
}
INIT_FUNCTION(init_umessage_input, net);

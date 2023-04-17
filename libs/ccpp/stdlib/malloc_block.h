#pragma once

#include <di/types/prelude.h>

namespace ccpp {
struct [[gnu::aligned(16)]] MallocBlock {
    usize block_size;
    usize block_alignment;
};
}

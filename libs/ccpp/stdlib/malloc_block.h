#pragma once

#include <di/prelude.h>

namespace ccpp {
struct [[gnu::aligned(16)]] MallocBlock {
    usize block_size;
    usize block_alignment;
};
}

#pragma once

#include <stdint.h>

namespace LIIM::Container::Hash {
class Hasher {
public:
    constexpr Hasher() {}

    constexpr uint64_t finish() { return m_running_hash; }

private:
    uint64_t m_running_hash { 0 };
};
}

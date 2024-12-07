#pragma once

#include "di/types/integers.h"
#include "di/vocab/error/result.h"
namespace di::io {
class SizeWriter {
private:
    template<typename U = void>
    using Result = vocab::Result<U>;

public:
    auto written() const { return m_written; }

    constexpr auto write_some(Span<byte const> bytes) -> Result<usize> {
        m_written += bytes.size();
        return bytes.size();
    }
    constexpr static auto flush() -> Result<> { return {}; }

private:
    usize m_written { 0 };
};
}

namespace di {
using io::SizeWriter;
}

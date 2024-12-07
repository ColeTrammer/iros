#pragma once

#include "di/any/concepts/impl.h"
#include "di/assert/assert_bool.h"
#include "di/container/string/encoding.h"
#include "di/container/string/transparent_encoding.h"
#include "di/container/string/utf8_encoding.h"
#include "di/io/interface/reader.h"

namespace di::io {
namespace detail {
    struct ReadExactlyFunction {
        template<concepts::Impl<Reader> Reader>
        constexpr auto operator()(Reader& readr, Span<byte> data) const -> meta::ReaderResult<void, Reader> {
            auto nread = usize { 0 };
            while (nread < data.size()) {
                auto n = DI_TRY(read_some(readr, *data.subspan(nread)));
                DI_ASSERT(n > 0);
                nread += n;
            }
            return {};
        };
    };
}

constexpr inline auto read_exactly = detail::ReadExactlyFunction {};
}

namespace di {
using io::read_exactly;
}

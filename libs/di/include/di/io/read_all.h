#pragma once

#include <di/container/vector/prelude.h>
#include <di/function/monad/monad_try.h>
#include <di/io/interface/reader.h>
#include <di/vocab/expected/prelude.h>

namespace di::io {
namespace detail {
    struct ReadAll {
        constexpr auto operator()(Impl<Reader> auto& reader) const -> Result<Vector<Byte>> {
            constexpr auto block_size = 16384ZU;

            auto buffer = Vector<Byte> {};

            auto try_reserve_more_capacity = [&] {
                return invoke_as_fallible([&] {
                    buffer.reserve(buffer.capacity() + block_size);
                    return;
                });
            };

            for (;;) {
                auto nread = DI_TRY(try_reserve_more_capacity() >> [&] {
                    return read_some(reader, { buffer.end(), buffer.begin() + buffer.capacity() });
                } | if_success([&](auto nread) {
                                        buffer.assume_size(buffer.size() + nread);
                                    }));
                if (nread == 0) {
                    break;
                }
            }

            return buffer;
        }
    };
}

constexpr inline auto read_all = detail::ReadAll {};
}

namespace di {
using io::read_all;
}

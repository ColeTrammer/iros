#pragma once

#include <di/container/vector/prelude.h>
#include <di/function/monad/monad_try.h>
#include <di/io/concepts/reader.h>
#include <di/vocab/expected/prelude.h>

namespace di::io {
namespace detail {
    struct ReadAll {
        constexpr Result<Vector<Byte>> operator()(concepts::Reader auto& reader) const {
            constexpr auto block_size = 16384zu;

            auto buffer = Vector<Byte> {};

            auto try_reserve_more_capacity = [&] {
                return invoke_as_fallible([&] {
                    return buffer.reserve(buffer.capacity() + block_size);
                });
            };

            for (;;) {
                auto nread = DI_TRY(try_reserve_more_capacity() >> [&] {
                    return reader.read({ buffer.end(), buffer.begin() + buffer.capacity() });
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
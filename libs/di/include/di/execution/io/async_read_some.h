#pragma once

#include "di/execution/concepts/scheduler.h"
#include "di/execution/concepts/sender_of.h"
#include "di/execution/receiver/set_value.h"
#include "di/execution/types/prelude.h"
#include "di/function/tag_invoke.h"
#include "di/meta/callable.h"
#include "di/types/integers.h"
#include "di/types/prelude.h"
#include "di/vocab/span/prelude.h"

namespace di::execution {
namespace async_read_some_ns {
    struct Function {
        template<typename File>
        requires(concepts::TagInvocable<Function, File, Span<Byte>, Optional<u64>>)
        auto operator()(File&& handle, Span<Byte> buffer, Optional<u64> offset = {}) const
            -> concepts::SenderOf<SetValue(usize)> auto {
            return function::tag_invoke(*this, util::forward<File>(handle), buffer, offset);
        }

        template<typename File>
        requires(concepts::TagInvocable<Function, File&, Span<Byte>, Optional<u64>>)
        auto operator()(util::ReferenceWrapper<File> handle, Span<Byte> buffer, Optional<u64> offset = {}) const {
            return function::tag_invoke(*this, handle.get(), buffer, offset);
        }
    };
}

constexpr inline auto async_read_some = async_read_some_ns::Function {};
}

namespace di::concepts {
template<typename T>
concept AsyncReadable = concepts::Callable<execution::async_read_some_ns::Function, T&, vocab::Span<byte>>;
}

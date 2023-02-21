#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct GenerateFunction {
        template<concepts::Iterator Out, concepts::SentinelFor<Out> Sent, concepts::CopyConstructible F>
        requires(concepts::Invocable<F&> && concepts::IndirectlyWritable<Out, meta::InvokeResult<F&>>)
        constexpr Out operator()(Out output, Sent last, F gen) const {
            for (; output != last; ++output) {
                *output = function::invoke(gen);
            }
            return output;
        }

        template<typename Con, concepts::CopyConstructible F>
        requires(concepts::Invocable<F&> && concepts::OutputContainer<Con, meta::InvokeResult<F&>>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, F gen) const {
            return (*this)(container::begin(container), container::end(container), util::ref(gen));
        }
    };
}

constexpr inline auto generate = detail::GenerateFunction {};
}

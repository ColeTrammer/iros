#pragma once

#include <di/container/algorithm/out_value_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct IotaFunction {
        template<concepts::Iterator Out, concepts::SentinelFor<Out> Sent, concepts::WeaklyIncrementable T>
        requires(concepts::IndirectlyWritable<Out, T const&>)
        constexpr OutValueResult<Out, T> operator()(Out output, Sent last, T value) const {
            for (; output != last; ++output, ++value) {
                *output = util::as_const(value);
            }
            return { util::move(output), util::move(value) };
        }

        template<concepts::WeaklyIncrementable T, concepts::OutputContainer<T const&> Con>
        constexpr OutValueResult<meta::BorrowedIterator<Con>, T> operator()(Con&& container, T value) const {
            return (*this)(container::begin(container), container::end(container), util::move(value));
        }
    };
}

constexpr inline auto iota = detail::IotaFunction {};
}

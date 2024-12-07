#pragma once

#include "di/container/algorithm/in_out_result.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"

namespace di::container {
namespace detail {
    struct FillFunction {
        template<typename T, concepts::OutputIterator<T const&> Out, concepts::SentinelFor<Out> Sent>
        constexpr auto operator()(Out first, Sent last, T const& value) const -> Out {
            for (; first != last; ++first) {
                *first = value;
            }
            return first;
        }

        template<typename T, concepts::OutputContainer<T const&> Out>
        constexpr auto operator()(Out&& container, T const& value) const -> meta::BorrowedIterator<Out> {
            return (*this)(container::begin(container), container::end(container), value);
        }
    };
}

constexpr inline auto fill = detail::FillFunction {};
}

namespace di {
using container::fill;
}

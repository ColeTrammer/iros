#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct FillNFunction {
        template<typename T, concepts::OutputIterator<T const&> Out, typename SSizeType = meta::IteratorSSizeType<Out>>
        constexpr auto operator()(Out first, meta::TypeIdentity<SSizeType> n, T const& value) const -> Out {
            for (SSizeType i = 0; i < n; ++i, ++first) {
                *first = value;
            }
            return first;
        }
    };
}

constexpr inline auto fill_n = detail::FillNFunction {};
}

namespace di {
using container::fill_n;
}

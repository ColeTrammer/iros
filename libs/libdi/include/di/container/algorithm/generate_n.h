#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct GenerateNFunction {
        template<concepts::Iterator Out, typename SSizeType = meta::IteratorSSizeType<Out>,
                 concepts::CopyConstructible F>
        requires(concepts::Invocable<F&> && concepts::IndirectlyWritable<Out, meta::InvokeResult<F&>>)
        constexpr Out operator()(Out output, meta::TypeIdentity<SSizeType> n, F gen) const {
            for (SSizeType i = 0; i < n; ++i, ++output) {
                *output = function::invoke(gen);
            }
            return output;
        }
    };
}

constexpr inline auto generate_n = detail::GenerateNFunction {};
}
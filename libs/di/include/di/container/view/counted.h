#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/counted_iterator.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view.h>
#include <di/function/tag_invoke.h>
#include <di/vocab/span/prelude.h>

namespace di::container::view {
namespace detail {
    struct CountedFunction;

    template<typename It, typename Diff>
    concept CustomCounted = concepts::TagInvocable<CountedFunction, It, Diff>;

    template<typename It, typename Diff>
    concept SpanCounted = concepts::ContiguousIterator<It>;

    template<typename It, typename Diff>
    concept ViewCounted = concepts::RandomAccessIterator<It>;

    struct CountedFunction {
        template<typename Iter, typename It = meta::Decay<Iter>,
                 concepts::ConvertibleTo<meta::IteratorSSizeType<It>> Diff>
        constexpr auto operator()(Iter&& it, Diff&& n_in) const -> concepts::View auto {
            using SSizeType = meta::IteratorSSizeType<It>;
            auto n = static_cast<SSizeType>(n_in);
            if constexpr (CustomCounted<It, Diff>) {
                return tag_invoke(*this, it, n);
            } else if constexpr (SpanCounted<It, Diff>) {
                return Span { util::to_address(it), static_cast<size_t>(n) };
            } else if constexpr (ViewCounted<It, Diff>) {
                return View { it, it + n };
            } else {
                return View { CountedIterator(it, n), default_sentinel };
            }
        }
    };
}

constexpr inline auto counted = detail::CountedFunction {};
}

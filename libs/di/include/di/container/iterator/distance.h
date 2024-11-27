#pragma once

#include <di/container/concepts/container.h>
#include <di/container/concepts/iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/interface/size.h>
#include <di/container/interface/ssize.h>
#include <di/container/meta/container_ssize_type.h>
#include <di/container/meta/iterator_ssize_type.h>

namespace di::container {
namespace detail {
    struct DistanceFunction {
        template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
        requires(!concepts::SizedSentinelFor<Sent, Iter>)
        constexpr auto operator()(Iter iter, Sent sent) const -> meta::IteratorSSizeType<Iter> {
            auto distance = meta::IteratorSSizeType<Iter> { 0 };
            for (; iter != sent; ++iter) {
                ++distance;
            }
            return distance;
        }

        template<concepts::Iterator Iter, concepts::SizedSentinelFor<Iter> Sent>
        constexpr auto operator()(Iter const& iter, Sent const& sent) const {
            return sent - iter;
        }

        template<concepts::Container Con>
        constexpr auto operator()(Con&& container) const -> meta::ContainerSSizeType<Con> {
            if constexpr (concepts::SizedContainer<Con>) {
                return container::ssize(container);
            } else {
                return (*this)(begin(container), end(container));
            }
        }
    };
}

constexpr inline auto distance = detail::DistanceFunction {};
}

namespace di {
using container::distance;
}

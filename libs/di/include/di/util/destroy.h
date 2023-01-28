#pragma once

#include <di/concepts/destructible.h>
#include <di/container/concepts/input_container.h>
#include <di/container/concepts/input_iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/meta/borrowed_iterator.h>
#include <di/container/meta/container_value.h>
#include <di/container/meta/iterator_value.h>
#include <di/util/destroy_at.h>

namespace di::util {
namespace detail {
    struct DestroyFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent>
        requires(concepts::Destructible<meta::IteratorValue<Iter>>)
        constexpr Iter operator()(Iter it, Sent sent) const {
            for (; it != sent; ++it) {
                destroy_at(util::address_of(*it));
            }
            return it;
        }

        template<concepts::InputContainer Con>
        requires(concepts::Destructible<meta::ContainerValue<Con>>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container) const {
            return (*this)(container::begin(container), container::end(container));
        }
    };
}

constexpr inline auto destroy = detail::DestroyFunction {};
}

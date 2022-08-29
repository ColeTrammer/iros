#pragma once

#include <di/concepts/class.h>
#include <di/concepts/same_as.h>
#include <di/container/concepts/bidirectional_container.h>
#include <di/container/concepts/common_container.h>
#include <di/container/concepts/contiguous_iterator.h>
#include <di/container/concepts/forward_container.h>
#include <di/container/concepts/has_empty_container.h>
#include <di/container/concepts/random_access_container.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/interface/empty.h>
#include <di/container/interface/size.h>
#include <di/container/iterator/prev.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_ssize_type.h>
#include <di/container/meta/container_value.h>
#include <di/container/meta/enable_view.h>
#include <di/meta/remove_cv.h>
#include <di/vocab/optional.h>

namespace di::container {
template<typename Self>
requires(concepts::Class<Self> && concepts::SameAs<Self, meta::RemoveCV<Self>>)
class ViewInterface : public meta::EnableView<Self> {
public:
    constexpr bool empty()
    requires(concepts::SizedContainer<Self> || concepts::ForwardContainer<Self>)
    {
        if constexpr (concepts::SizedContainer<Self>) {
            return container::size(self()) == 0;
        } else {
            return container::begin(self()) == container::end(self());
        }
    }

    constexpr bool empty() const
    requires(concepts::SizedContainer<Self const> || concepts::ForwardContainer<Self const>)
    {
        if constexpr (concepts::SizedContainer<Self>) {
            return container::size(self()) == 0;
        } else {
            return container::begin(self()) == container::end(self());
        }
    }

    constexpr explicit operator bool()
    requires(concepts::HasEmptyContainer<Self>)
    {
        return !container::empty(self);
    }

    constexpr explicit operator bool() const
    requires(concepts::HasEmptyContainer<Self const>)
    {
        return !container::empty(self);
    }

    constexpr auto data()
    requires(concepts::ContiguousIterator<meta::ContainerIterator<Self>>)
    {
        return container::begin(self());
    }

    constexpr auto data() const
    requires(concepts::Container<Self const> && concepts::ContiguousIterator<meta::ContainerIterator<Self const>>)
    {
        return container::begin(self());
    }

    constexpr auto size()
    requires(concepts::ForwardContainer<Self> && concepts::SizedSentinelFor<meta::ContainerIterator<Self>, meta::ContainerSentinel<Self>>)
    {
        return container::end(self()) - container::begin(self());
    }

    constexpr auto size() const
    requires(concepts::ForwardContainer<Self const> &&
             concepts::SizedSentinelFor<meta::ContainerIterator<Self const>, meta::ContainerSentinel<Self const>>)
    {
        return container::end(self()) - container::begin(self());
    }

    constexpr auto front()
    requires(concepts::ForwardContainer<Self>)
    {
        using Result = vocab::Optional<meta::ContainerValue<Self>>;
        if (this->empty()) {
            return Result(vocab::nullopt);
        } else {
            return Result(*container::begin(self()));
        }
    }

    constexpr auto front() const
    requires(concepts::ForwardContainer<Self const>)
    {
        using Result = vocab::Optional<meta::ContainerValue<Self const>>;
        if (this->empty()) {
            return Result(vocab::nullopt);
        } else {
            return Result(*container::begin(self()));
        }
    }

    constexpr auto back()
    requires(concepts::BidirectionalContainer<Self> && concepts::CommonContainer<Self>)
    {
        using Result = vocab::Optional<meta::ContainerValue<Self>>;
        if (this->empty()) {
            return Result(vocab::nullopt);
        } else {
            return Result(*container::prev(container::end(self())));
        }
    }

    constexpr auto back() const
    requires(concepts::BidirectionalContainer<Self const> && concepts::CommonContainer<Self const>)
    {
        using Result = vocab::Optional<meta::ContainerValue<Self const>>;
        if (this->empty()) {
            return Result(vocab::nullopt);
        } else {
            return Result(*container::prev(container::end(self())));
        }
    }

    template<concepts::RandomAccessContainer Cont = Self>
    constexpr decltype(auto) operator[](meta::ContainerSSizeType<Cont> n) {
        return container::begin(self())[n];
    }

    template<concepts::RandomAccessContainer Cont = Self const>
    constexpr decltype(auto) operator[](meta::ContainerSSizeType<Cont> n) const {
        return container::begin(self())[n];
    }

private:
    Self& self() { return static_cast<Self&>(*this); }
    Self const& self() const { return static_cast<Self const&>(*this); }
};
}

#pragma once

#include <di/container/concepts/bidirectional_container.h>
#include <di/container/concepts/common_container.h>
#include <di/container/concepts/contiguous_iterator.h>
#include <di/container/concepts/forward_container.h>
#include <di/container/concepts/has_empty_container.h>
#include <di/container/concepts/random_access_container.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/interface/cbegin.h>
#include <di/container/interface/cend.h>
#include <di/container/interface/empty.h>
#include <di/container/interface/size.h>
#include <di/container/iterator/prev.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_reference.h>
#include <di/container/meta/container_ssize_type.h>
#include <di/container/meta/enable_view.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/vocab/optional/prelude.h>

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
        return !container::empty(self());
    }

    constexpr explicit operator bool() const
    requires(concepts::HasEmptyContainer<Self const>)
    {
        return !container::empty(self());
    }

    constexpr auto cbegin() { return container::cbegin(self()); }
    constexpr auto cbegin() const
    requires(concepts::Container<Self const>)
    {
        return container::cbegin(self());
    }

    constexpr auto cend() { return container::cend(self()); }
    constexpr auto cend() const
    requires(concepts::Container<Self const>)
    {
        return container::cend(self());
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
    requires(concepts::ForwardContainer<Self> &&
             concepts::SizedSentinelFor<meta::ContainerIterator<Self>, meta::ContainerSentinel<Self>>)
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
        using Result = vocab::Optional<meta::ContainerReference<Self>>;
        if (this->empty()) {
            return Result(vocab::nullopt);
        } else {
            return Result(*container::begin(self()));
        }
    }

    constexpr auto front() const
    requires(concepts::ForwardContainer<Self const>)
    {
        using Result = vocab::Optional<meta::ContainerReference<Self const>>;
        if (this->empty()) {
            return Result(vocab::nullopt);
        } else {
            return Result(*container::begin(self()));
        }
    }

    constexpr auto back()
    requires(concepts::BidirectionalContainer<Self> && concepts::CommonContainer<Self>)
    {
        using Result = vocab::Optional<meta::ContainerReference<Self>>;
        if (this->empty()) {
            return Result(vocab::nullopt);
        } else {
            return Result(*container::prev(container::end(self())));
        }
    }

    constexpr auto back() const
    requires(concepts::BidirectionalContainer<Self const> && concepts::CommonContainer<Self const>)
    {
        using Result = vocab::Optional<meta::ContainerReference<Self const>>;
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

    template<concepts::RandomAccessContainer Cont = Self>
    constexpr auto at(meta::ContainerSSizeType<Cont> n) -> Optional<meta::ContainerReference<Cont>> {
        if (n < 0 || n >= this->size()) {
            return vocab::nullopt;
        }
        return container::begin(self())[n];
    }

    template<concepts::RandomAccessContainer Cont = Self const>
    constexpr auto at(meta::ContainerSSizeType<Cont> n) const -> Optional<meta::ContainerReference<Cont>> {
        if (n < 0 || n >= this->size()) {
            return vocab::nullopt;
        }
        return container::begin(self())[n];
    }

private:
    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }
};
}

namespace di {
namespace container::view {}

namespace view = container::view;
}

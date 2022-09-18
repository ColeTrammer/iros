#pragma once

#include <di/concepts/can_reference.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_initializable.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/object.h>
#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/function/invoke.h>
#include <di/meta/maybe_const.h>
#include <di/meta/remove_cvref.h>
#include <di/util/move.h>
#include <di/util/rebindable_box.h>

namespace di::container {
template<concepts::InputContainer View, concepts::MoveConstructible F>
requires(concepts::View<View> && concepts::Object<F> && concepts::Invocable<F&, meta::ContainerReference<View>> &&
         concepts::CanReference<meta::InvokeResult<F&, meta::ContainerReference<View>>>)
class TransformView : public ViewInterface<TransformView<View, F>> {
private:
    template<bool is_const>
    class Sentinel;

    template<bool is_const>
    class Iterator;

    template<bool is_const>
    class Sentinel {
    private:
        using Base = meta::MaybeConst<is_const, View>;
        using Sent = meta::ContainerSentinel<Base>;
        using SSizeType = meta::ContainerSSizeType<Base>;

    public:
        constexpr Sentinel() = default;

        constexpr explicit Sentinel(Sent sentinel) : m_sentinel(sentinel) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<meta::ContainerSentinel<View>, Sent>)
            : m_sentinel(other.base()) {}

        constexpr auto base() const { return m_sentinel; }

    private:
        constexpr friend bool operator==(Iterator<is_const> const& a, Sentinel const& b) { return a.base() == b.base(); }

        constexpr friend SSizeType operator-(Iterator<is_const> const& a, Sentinel const& b)
        requires(concepts::SizedSentinelFor<Sent, meta::ContainerIterator<Base>>)
        {
            return a.base() - b.base();
        }

        constexpr friend SSizeType operator-(Sentinel const& a, Iterator<is_const> const& b)
        requires(concepts::SizedSentinelFor<Sent, meta::ContainerIterator<Base>>)
        {
            return b.base() - a.base();
        }

        Sent m_sentinel {};
    };

    template<bool is_const>
    class Iterator {
    private:
        using Base = meta::MaybeConst<is_const, View>;
        using Parent = meta::MaybeConst<is_const, TransformView>;
        using Iter = meta::ContainerIterator<Base>;
        using SSizeType = meta::ContainerSSizeType<Base>;

    public:
        Iterator()
        requires(concepts::DefaultInitializable<Iter>)
        = default;

        constexpr explicit Iterator(Parent& parent, Iter iterator) : m_iterator(util::move(iterator)), m_parent(util::address_of(parent)) {}

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter, meta::ContainerIterator<Base>>)
            : m_iterator(util::move(other).base()), m_parent(other.m_parent) {}

        constexpr Iter const& base() const& { return m_iterator; }
        constexpr Iter base() && { return util::move(m_iterator); }

        constexpr decltype(auto) operator*() const { return function::invoke(m_parent->m_function.value(), *m_iterator); }

        constexpr decltype(auto) operator[](SSizeType n) const
        requires(concepts::RandomAccessContainer<Base>)
        {
            return function::invoke(m_parent->m_function.value(), m_iterator[n]);
        }

        constexpr Iterator& operator++() {
            ++m_iterator;
            return *this;
        }

        constexpr void operator++(int) { ++m_iterator; }
        constexpr Iterator operator++(int)
        requires(concepts::ForwardContainer<Base>)
        {
            ++m_iterator;
            return *this;
        }

    private:
        template<bool>
        friend class Iterator;

        constexpr friend bool operator==(Iterator const& a, Iterator const& b)
        requires(concepts::EqualityComparable<Iter>)
        {
            return a.base() == b.base();
        }

        constexpr friend auto tag_invoke(types::Tag<iterator_category>, types::InPlaceType<Iterator>) {
            if constexpr (concepts::RandomAccessContainer<Base>) {
                return types::RandomAccessIteratorTag {};
            } else if constexpr (concepts::BidirectionalContainer<Base>) {
                return types::BidirectionalIteratorTag {};
            } else if constexpr (concepts::ForwardContainer<Base>) {
                return types::ForwardIteratorTag {};
            } else {
                return types::InputIteratorTag {};
            }
        }

        using Reference = meta::InvokeResult<meta::MaybeConst<is_const, F>&, meta::ContainerReference<Base>>;
        constexpr friend meta::RemoveCVRef<Reference> tag_invoke(types::Tag<iterator_value>, types::InPlaceType<Iterator>) {}
        constexpr friend SSizeType tag_invoke(types::Tag<iterator_ssize_type>, types::InPlaceType<Iterator>) {}

        Iter m_iterator {};
        Parent* m_parent { nullptr };
    };

public:
    constexpr TransformView()
    requires(concepts::DefaultInitializable<View> && concepts::DefaultInitializable<F>)
    = default;

    constexpr TransformView(View base, F function) : m_base(util::move(base)), m_function(util::move(function)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }

    constexpr View base() && { return util::move(m_base); }

    constexpr auto begin() { return Iterator<false>(*this, container::begin(m_base)); }

    constexpr auto begin() const
    requires(concepts::Container<View const> && concepts::Invocable<F const&, meta::ContainerReference<View const>>)
    {
        return Iterator<true>(*this, container::begin(m_base));
    }

    constexpr auto end() { return Sentinel<false>(container::end(m_base)); }
    constexpr auto end()
    requires(concepts::CommonContainer<View>)
    {
        return Iterator<false>(*this, container::end(m_base));
    }

    constexpr auto end() const
    requires(concepts::Container<View const> && concepts::Invocable<F const&, meta::ContainerReference<View const>>)
    {
        return Sentinel<true>(container::begin(m_base));
    }

    constexpr auto end() const
    requires(concepts::CommonContainer<View> && concepts::Container<View const> &&
             concepts::Invocable<F const&, meta::ContainerReference<View const>>)
    {
        return Iterator<true>(*this, container::end(m_base));
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        return container::size(m_base);
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        return container::size(m_base);
    }

private:
    View m_base {};
    util::RebindableBox<F> m_function {};
};

template<typename Con, typename F>
TransformView(Con&&, F) -> TransformView<meta::AsView<Con>, F>;
}

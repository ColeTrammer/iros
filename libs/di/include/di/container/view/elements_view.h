#pragma once

#include <di/container/concepts/borrowed_container.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_extension.h>
#include <di/container/iterator/sentinel_extension.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/meta/maybe_const.h>
#include <di/meta/remove_cv.h>
#include <di/util/move.h>
#include <di/vocab/tuple/prelude.h>

namespace di::container {
namespace detail {
    template<typename T, size_t index>
    concept ReturnableElement = concepts::Reference<T> || concepts::MoveConstructible<meta::TupleElement<T, index>>;
}

template<concepts::InputContainer View, size_t index>
requires(concepts::View<View> && concepts::TupleLike<meta::ContainerValue<View>> &&
         concepts::TupleLike<meta::ContainerReference<View>>)
class ElementsView
    : public ViewInterface<ElementsView<View, index>>
    , public meta::EnableBorrowedContainer<ElementsView<View, index>, concepts::BorrowedContainer<View>> {
private:
    template<bool is_const>
    using Iter = meta::ContainerIterator<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using Value = meta::ContainerValue<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    class Iterator
        : public IteratorExtension<Iterator<is_const>, Iter<is_const>,
                                   meta::RemoveCVRef<meta::TupleElement<Value<is_const>, index>>> {
        using Base = IteratorExtension<Iterator<is_const>, Iter<is_const>,
                                       meta::RemoveCVRef<meta::TupleElement<Value<is_const>, index>>>;

    public:
        Iterator()
        requires(concepts::DefaultInitializable<Iter<is_const>>)
        = default;

        constexpr explicit Iterator(Iter<is_const> base) : Base(util::move(base)) {}

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<false>, Iter<true>>)
            : Base(util::move(other).base()) {}

        constexpr decltype(auto) operator*() const {
            if constexpr (concepts::Reference<meta::IteratorReference<Iter<is_const>>>) {
                return util::get<index>(*this->base());
            } else {
                using R = meta::RemoveCV<meta::TupleElement<meta::IteratorReference<Iter<is_const>>, index>>;
                return static_cast<R>(util::get<index>(*this->base()));
            }
        }
    };

    template<bool is_const>
    class Sentinel : public SentinelExtension<Sentinel<is_const>, Sent<is_const>, Iterator<is_const>, Iter<is_const>> {
    private:
        using Base = SentinelExtension<Sentinel<is_const>, Sent<is_const>, Iterator<is_const>, Iter<is_const>>;

    public:
        Sentinel() = default;

        constexpr explicit Sentinel(Sent<is_const> base) : Base(base) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : Base(other.base()) {}
    };

public:
    ElementsView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit ElementsView(View base) : m_base(util::move(base)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View>)
    {
        return Iterator<false>(container::begin(m_base));
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        return Iterator<true>(container::begin(m_base));
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (!concepts::CommonContainer<View>) {
            return Sentinel<false>(container::end(m_base));
        } else {
            return Iterator<false>(container::end(m_base));
        }
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        if constexpr (!concepts::CommonContainer<View const>) {
            return Sentinel<true>(container::end(m_base));
        } else {
            return Iterator<true>(container::end(m_base));
        }
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
};
}

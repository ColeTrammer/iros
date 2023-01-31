#pragma once

#include <di/concepts/can_reference.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_initializable.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/object.h>
#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/iterator/iterator_extension.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/iterator/sentinel_extension.h>
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
    using Base = meta::MaybeConst<is_const, View>;

    template<bool is_const>
    using Iter = meta::ContainerIterator<Base<is_const>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<Base<is_const>>;

    template<bool is_const>
    using SSizeType = meta::ContainerSSizeType<Base<is_const>>;

    template<bool is_const>
    using Parent = meta::MaybeConst<is_const, TransformView>;

    template<bool is_const>
    class Sentinel;

    template<bool is_const>
    class Iterator;

    template<bool is_const>
    class Sentinel : public SentinelExtension<Sentinel<is_const>, Sent<is_const>, Iterator<is_const>, Iter<is_const>> {
    private:
        using Base = SentinelExtension<Sentinel<is_const>, Sent<is_const>, Iterator<is_const>, Iter<is_const>>;

    public:
        constexpr Sentinel() = default;

        constexpr explicit Sentinel(Sent<is_const> sentinel) : Base(sentinel) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Sent<is_const>, Sent<!is_const>>)
            : Base(other.base()) {}
    };

    template<bool is_const>
    class Iterator
        : public IteratorExtension<Iterator<is_const>, Iter<is_const>,
                                   meta::RemoveCVRef<meta::InvokeResult<meta::MaybeConst<is_const, F>&,
                                                                        meta::IteratorReference<Iter<is_const>>>>> {
        using Base = IteratorExtension<Iterator<is_const>, Iter<is_const>,
                                       meta::RemoveCVRef<meta::InvokeResult<meta::MaybeConst<is_const, F>&,
                                                                            meta::IteratorReference<Iter<is_const>>>>>;

    public:
        Iterator()
        requires(concepts::DefaultInitializable<Iter<is_const>>)
        = default;

        constexpr explicit Iterator(Parent<is_const>& parent, Iter<is_const> iterator)
            : Base(util::move(iterator)), m_parent(util::addressof(parent)) {}

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<is_const>, Iter<!is_const>>)
            : Base(util::move(other).base()), m_parent(other.m_parent) {}

        constexpr decltype(auto) operator*() const {
            return function::invoke(m_parent->m_function.value(), *this->base());
        }

    private:
        template<bool>
        friend class Iterator;

        constexpr friend decltype(auto) tag_invoke(types::Tag<iterator_move>, Iterator const& self) {
            if constexpr (concepts::LValueReference<decltype(*self)>) {
                return util::move(*self);
            } else {
                return *self;
            }
        }

        Parent<is_const>* m_parent { nullptr };
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
        return Sentinel<true>(container::end(m_base));
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

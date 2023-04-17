#pragma once

#include <di/concepts/can_reference.h>
#include <di/container/iterator/iterator_extension.h>
#include <di/container/iterator/sentinel_extension.h>
#include <di/container/view/adjacent_view.h>
#include <di/util/rebindable_box.h>

namespace di::container {
namespace detail {
    template<typename F, typename P>
    struct CanInvokeRepeatHelper : meta::FalseType {};

    template<typename F, typename... Rs>
    requires(concepts::Invocable<F, Rs...>)
    struct CanInvokeRepeatHelper<F, meta::List<Rs...>> : meta::TrueType {
        using Type = meta::InvokeResult<F, Rs...>;
    };

    template<typename F, typename V, size_t N>
    concept CanInvokeRepeat = CanInvokeRepeatHelper<F, meta::Repeat<V, N>>::value;

    template<typename F, typename V, size_t N>
    using InvokeRepeatResult = CanInvokeRepeatHelper<F, meta::Repeat<V, N>>::Type;
}

template<concepts::InputContainer View, concepts::MoveConstructible F, size_t N>
requires(concepts::View<View> && N > 0 && detail::CanInvokeRepeat<F&, meta::ContainerReference<View>, N> &&
         concepts::CanReference<detail::InvokeRepeatResult<F&, meta::ContainerReference<View>, N>>)
class AdjacentTransformView : public ViewInterface<AdjacentTransformView<View, F, N>> {
private:
    using InnerView = AdjacentView<View, N>;

    template<bool is_const>
    using InnerIterator = meta::ContainerIterator<meta::MaybeConst<is_const, InnerView>>;

    template<bool is_const>
    using InnerSentinel = meta::ContainerSentinel<meta::MaybeConst<is_const, InnerView>>;

    template<bool is_const>
    class Sentinel;

    template<bool is_const>
    class Iterator;

    template<bool is_const>
    class Sentinel
        : public SentinelExtension<Sentinel<is_const>, InnerSentinel<is_const>, Iterator<is_const>,
                                   InnerIterator<is_const>> {
    private:
        using Base =
            SentinelExtension<Sentinel<is_const>, InnerSentinel<is_const>, Iterator<is_const>, InnerIterator<is_const>>;

        friend class AdjacentTransformView;

        constexpr explicit Sentinel(InnerSentinel<is_const> sentinel) : Base(sentinel) {}

    public:
        constexpr Sentinel() = default;

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<InnerSentinel<is_const>, InnerSentinel<!is_const>>)
            : Base(other.base()) {}
    };

    template<bool is_const>
    class Iterator
        : public IteratorExtension<
              Iterator<is_const>, InnerIterator<is_const>,
              meta::RemoveCVRef<detail::InvokeRepeatResult<
                  meta::MaybeConst<is_const, F>&, meta::ContainerReference<meta::MaybeConst<is_const, View>>, N>>> {
        using Base = IteratorExtension<
            Iterator<is_const>, InnerIterator<is_const>,
            meta::RemoveCVRef<detail::InvokeRepeatResult<
                meta::MaybeConst<is_const, F>&, meta::ContainerReference<meta::MaybeConst<is_const, View>>, N>>>;

        friend class AdjacentTransformView;

        constexpr explicit Iterator(meta::MaybeConst<is_const, AdjacentTransformView>& parent,
                                    InnerIterator<is_const> iterator)
            : Base(util::move(iterator)), m_parent(util::addressof(parent)) {}

    public:
        Iterator()
        requires(concepts::DefaultInitializable<InnerIterator<is_const>>)
        = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<InnerIterator<is_const>, InnerIterator<!is_const>>)
            : Base(util::move(other).base()), m_parent(other.m_parent) {}

        constexpr decltype(auto) operator*() const {
            return apply(
                [&](auto const&... iters) -> decltype(auto) {
                    return function::invoke(m_parent->m_function.value(), *iters...);
                },
                this->base().iterators());
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

        meta::MaybeConst<is_const, AdjacentTransformView>* m_parent { nullptr };
    };

public:
    AdjacentTransformView() = default;

    constexpr explicit AdjacentTransformView(View view, F function)
        : m_inner(util::move(view)), m_function(util::move(function)) {}

    constexpr auto begin() { return Iterator<false>(*this, m_inner.begin()); }

    constexpr auto begin() const
    requires(concepts::Container<InnerView const> &&
             detail::CanInvokeRepeat<F const&, meta::ContainerReference<View const>, N>)
    {
        return Iterator<true>(*this, m_inner.begin());
    }

    constexpr auto end() {
        if constexpr (concepts::CommonContainer<InnerView>) {
            return Iterator<false>(*this, m_inner.end());
        } else {
            return Sentinel<false>(m_inner.end());
        }
    }

    constexpr auto end() const
    requires(concepts::Container<InnerView const> &&
             detail::CanInvokeRepeat<F const&, meta::ContainerReference<View const>, N>)
    {
        if constexpr (concepts::CommonContainer<InnerView const>) {
            return Iterator<true>(*this, m_inner.end());
        } else {
            return Sentinel<true>(m_inner.end());
        }
    }

    constexpr auto size()
    requires(concepts::SizedContainer<InnerView>)
    {
        return m_inner.size();
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<InnerView const>)
    {
        return m_inner.size();
    }

private:
    InnerView m_inner;
    util::RebindableBox<F> m_function;
};
}

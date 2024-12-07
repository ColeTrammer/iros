#pragma once

#include "di/container/iterator/iterator_extension.h"
#include "di/container/iterator/sentinel_extension.h"
#include "di/container/view/zip_view.h"
#include "di/meta/operations.h"
#include "di/util/rebindable_box.h"

namespace di::container {
template<concepts::MoveConstructible F, concepts::InputContainer... Views>
requires((concepts::View<Views> && ...) && sizeof...(Views) > 0 &&
         concepts::Invocable<F&, meta::ContainerReference<Views>...> &&
         concepts::CanReference<meta::InvokeResult<F&, meta::ContainerReference<Views>...>>)
class ZipTransformView : public ViewInterface<ZipTransformView<F, Views...>> {
private:
    using InnerView = ZipView<Views...>;

    template<bool is_const>
    using Ziperator = meta::ContainerIterator<meta::MaybeConst<is_const, InnerView>>;

    template<bool is_const>
    using Zentinel = meta::ContainerSentinel<meta::MaybeConst<is_const, InnerView>>;

    template<bool is_const>
    class Sentinel;

    template<bool is_const>
    class Iterator;

    template<bool is_const>
    class Sentinel
        : public SentinelExtension<Sentinel<is_const>, Zentinel<is_const>, Iterator<is_const>, Ziperator<is_const>> {
    private:
        using Base = SentinelExtension<Sentinel<is_const>, Zentinel<is_const>, Iterator<is_const>, Ziperator<is_const>>;

        friend class ZipTransformView;

        constexpr explicit Sentinel(Zentinel<is_const> sentinel) : Base(sentinel) {}

    public:
        constexpr Sentinel() = default;

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Zentinel<is_const>, Zentinel<!is_const>>)
            : Base(other.base()) {}
    };

    template<bool is_const>
    class Iterator
        : public IteratorExtension<
              Iterator<is_const>, Ziperator<is_const>,
              meta::RemoveCVRef<meta::InvokeResult<meta::MaybeConst<is_const, F>&,
                                                   meta::ContainerReference<meta::MaybeConst<is_const, Views>>...>>> {
        using Base = IteratorExtension<
            Iterator<is_const>, Ziperator<is_const>,
            meta::RemoveCVRef<meta::InvokeResult<meta::MaybeConst<is_const, F>&,
                                                 meta::ContainerReference<meta::MaybeConst<is_const, Views>>...>>>;

        friend class ZipTransformView;

        constexpr explicit Iterator(meta::MaybeConst<is_const, ZipTransformView>& parent, Ziperator<is_const> iterator)
            : Base(util::move(iterator)), m_parent(util::addressof(parent)) {}

    public:
        Iterator()
        requires(concepts::DefaultInitializable<Ziperator<is_const>>)
        = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Ziperator<is_const>, Ziperator<!is_const>>)
            : Base(util::move(other).base()), m_parent(other.m_parent) {}

        constexpr auto operator*() const -> decltype(auto) {
            return apply(
                [&](auto const&... iters) -> decltype(auto) {
                    return function::invoke(m_parent->m_function.value(), *iters...);
                },
                this->base().iterators());
        }

    private:
        template<bool>
        friend class Iterator;

        constexpr friend auto tag_invoke(types::Tag<iterator_move>, Iterator const& self) -> decltype(auto) {
            if constexpr (concepts::LValueReference<decltype(*self)>) {
                return util::move(*self);
            } else {
                return *self;
            }
        }

        meta::MaybeConst<is_const, ZipTransformView>* m_parent { nullptr };
    };

public:
    ZipTransformView() = default;

    constexpr explicit ZipTransformView(F function, Views... views)
        : m_function(util::move(function)), m_zip(util::move(views)...) {}

    constexpr auto begin() { return Iterator<false>(*this, m_zip.begin()); }

    constexpr auto begin() const
    requires(concepts::Container<InnerView const> &&
             concepts::Invocable<F const&, meta::ContainerReference<Views const>...>)
    {
        return Iterator<true>(*this, m_zip.begin());
    }

    constexpr auto end() {
        if constexpr (concepts::CommonContainer<InnerView>) {
            return Iterator<false>(*this, m_zip.end());
        } else {
            return Sentinel<false>(m_zip.end());
        }
    }

    constexpr auto end() const
    requires(concepts::Container<InnerView const> &&
             concepts::Invocable<F const&, meta::ContainerReference<Views const>...>)
    {
        if constexpr (concepts::CommonContainer<InnerView const>) {
            return Iterator<true>(*this, m_zip.end());
        } else {
            return Sentinel<true>(m_zip.end());
        }
    }

    constexpr auto size()
    requires(concepts::SizedContainer<InnerView>)
    {
        return m_zip.size();
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<InnerView const>)
    {
        return m_zip.size();
    }

private:
    util::RebindableBox<F> m_function;
    ZipView<Views...> m_zip;
};

template<class F, class... Cons>
ZipTransformView(F, Cons&&...) -> ZipTransformView<F, meta::AsView<Cons>...>;
}

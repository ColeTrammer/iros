#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/function/invoke.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/util/addressof.h>
#include <di/util/rebindable_box.h>

namespace di::container {
template<concepts::View View, typename Pred>
requires(concepts::InputContainer<View> && concepts::Object<Pred> &&
         concepts::IndirectUnaryPredicate<Pred const, meta::ContainerIterator<View>>)
class TakeWhileView : public ViewInterface<TakeWhileView<View, Pred>> {
private:
    template<bool is_const>
    using Iter = meta::ContainerIterator<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    class Sentinel {
    public:
        Sentinel() = default;

        constexpr explicit Sentinel(Sent<is_const> base, Pred const* predicate)
            : m_base(base), m_predicate(predicate) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : m_base(other.base()), m_predicate(other.m_predicate) {}

        constexpr Sent<is_const> base() const { return m_base; }

    private:
        constexpr friend bool operator==(Iter<is_const> const& a, Sentinel const& b) {
            return a == b.base() || !function::invoke(*b.m_predicate, *a);
        }

        Sent<is_const> m_base {};
        Pred const* m_predicate { nullptr };
    };

public:
    TakeWhileView()
    requires(concepts::DefaultInitializable<View> && concepts::DefaultInitializable<Pred>)
    = default;

    constexpr explicit TakeWhileView(View base, Pred predicate)
        : m_base(util::move(base)), m_predicate(util::move(predicate)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr Pred const& pred() const { return m_predicate.value(); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View>)
    {
        return container::begin(m_base);
    }

    constexpr auto begin() const
    requires(concepts::Container<View const> && concepts::IndirectUnaryPredicate<Pred const, Iter<true>>)
    {
        return container::begin(m_base);
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        return Sentinel<false>(container::end(m_base), util::addressof(m_predicate.value()));
    }

    constexpr auto end() const
    requires(concepts::Container<View const> && concepts::IndirectUnaryPredicate<Pred const, Iter<true>>)
    {
        return Sentinel<true>(container::end(m_base), util::addressof(m_predicate.value()));
    }

private:
    View m_base {};
    util::RebindableBox<Pred> m_predicate;
};

template<typename Con, typename Pred>
TakeWhileView(Con&&, Pred) -> TakeWhileView<meta::AsView<Con>, Pred>;
}

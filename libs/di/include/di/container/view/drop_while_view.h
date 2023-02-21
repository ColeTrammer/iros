#pragma once

#include <di/container/algorithm/find_if_not.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/prelude.h>
#include <di/function/invoke.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/reference_wrapper.h>
#include <di/util/store_if.h>

namespace di::container {
template<concepts::View View, typename Pred>
requires(concepts::InputContainer<View> && concepts::Object<Pred> &&
         concepts::IndirectUnaryPredicate<Pred const, meta::ContainerIterator<View>>)
class DropWhileView
    : public ViewInterface<DropWhileView<View, Pred>>
    , public meta::EnableBorrowedContainer<DropWhileView<View, Pred>, concepts::BorrowedContainer<View>> {
public:
    DropWhileView()
    requires(concepts::DefaultInitializable<View> && concepts::DefaultInitializable<Pred>)
    = default;

    constexpr explicit DropWhileView(View base, Pred predicate)
        : m_base(util::move(base)), m_predicate(util::move(predicate)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr Pred const& pred() const { return m_predicate.value(); }

    constexpr auto begin() {
        if constexpr (!concepts::ForwardContainer<View>) {
            return container::find_if_not(m_base, util::cref(m_predicate.value()));
        } else {
            if (!m_cache.value.has_value()) {
                m_cache.value = container::find_if_not(m_base, util::cref(m_predicate.value()));
            }
            return m_cache.value.value();
        }
    }

    constexpr auto end() { return container::end(m_base); }

private:
    View m_base {};
    util::RebindableBox<Pred> m_predicate;
    util::StoreIf<util::NonPropagatingCache<meta::ContainerIterator<View>>, concepts::ForwardContainer<View>> m_cache;
};

template<typename Con, typename Pred>
DropWhileView(Con&&, Pred) -> DropWhileView<meta::AsView<Con>, Pred>;
}

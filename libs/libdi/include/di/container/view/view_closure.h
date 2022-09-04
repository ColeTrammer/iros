#pragma once

#include <di/concepts/decay_same_as.h>
#include <di/concepts/derived_from.h>
#include <di/container/concepts/container.h>
#include <di/meta/decay.h>
#include <di/meta/index_sequence_for.h>
#include <di/meta/remove_cvref.h>
#include <di/util/bind_back.h>
#include <di/util/forward.h>
#include <di/util/invoke.h>

namespace di::container::view_adapter {
struct EnableViewClosure {};

template<typename T>
concept IsViewClosure = concepts::DerivedFrom<meta::RemoveCVRef<T>, EnableViewClosure> && !
concepts::Container<T>;

template<typename Fun, typename... BoundArgs>
struct ViewClosure
    : util::detail::BindBackFunction<meta::IndexSequenceFor<BoundArgs...>, Fun, BoundArgs...>
    , EnableViewClosure {
private:
    using Base = util::detail::BindBackFunction<meta::IndexSequenceFor<BoundArgs...>, Fun, BoundArgs...>;

public:
    using Base::Base;
    using Base::operator();
};

template<typename Fun, typename... Args>
constexpr auto view_closure(Fun&& function, Args&&... args) {
    return ViewClosure<meta::Decay<Fun>, meta::Decay<Args>...>(types::in_place, util::forward<Fun>(function), util::forward<Args>(args)...);
}

// This definitions ensures that expressions of the form:
//     container | di::container::view::thing(...) are valid.
template<concepts::Container Con, IsViewClosure Self>
requires(concepts::Invocable<Self, Con>)
constexpr decltype(auto) operator|(Con&& container, Self&& self) {
    return util::invoke(util::forward<Self>(self), util::forward<Con>(container));
}

// This definition ensures that expressions of the form:
//      container | (di::container::view::a(...) | di::container::view::b(...)) are valid.
template<IsViewClosure F, IsViewClosure G>
constexpr auto operator|(F&& f, G&& g) {
    return view_closure([f = util::forward<F>(f), g = util::forward<G>(g)](concepts::Container auto&& container)
                            -> decltype(util::invoke(g, util::invoke(f, util::forward<decltype(container)>(container)))) {
        return util::invoke(g, util::invoke(f, util::forward<decltype(container)>(container)));
    });
}
}

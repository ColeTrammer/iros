#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place.h>
#include <di/util/forward.h>
#include <di/util/move.h>
#include <di/vocab/tuple/apply.h>
#include <di/vocab/tuple/forward_as_tuple.h>
#include <di/vocab/tuple/tuple_cat.h>
#include <di/vocab/tuple/tuple_like.h>

namespace di::function::detail {
template<concepts::TupleLike Tup>
constexpr inline auto tuple_forward(Tup&& tuple) {
    return di::apply(
        []<typename... Args>(Args&&... args) {
            return di::forward_as_tuple<Args...>(di::forward<Args>(args)...);
        },
        di::forward<Tup>(tuple));
}

template<typename T>
constexpr inline auto tuple_forward(T&& value) {
    return di::forward_as_tuple<T>(di::forward<T>(value));
}

template<typename... Args>
constexpr inline auto do_tuple_cat(Args&&... args) {
    return di::tuple_cat(detail::tuple_forward(di::forward<Args>(args))...);
}

template<typename... Args>
using AsTuple = decltype(detail::do_tuple_cat(di::declval<Args>()...));

template<typename F, typename Tup>
concept TupleInvoceable =
    requires(F&& function, Tup&& tuple) { di::apply(di::forward<F>(function), di::forward<Tup>(tuple)); };

template<typename F, typename... Args>
concept CanUncurry = TupleInvoceable<F, AsTuple<Args...>>;

template<typename F>
class UncurryImpl : public pipeline::EnablePipeline {
private:
    F m_function;

public:
    template<typename Fn>
    constexpr UncurryImpl(types::InPlace, Fn&& function) : m_function(di::forward<Fn>(function)) {}

    constexpr UncurryImpl(UncurryImpl const&) = default;
    constexpr UncurryImpl(UncurryImpl&&) = default;

    constexpr auto operator=(UncurryImpl const&) -> UncurryImpl& = delete;
    constexpr auto operator=(UncurryImpl&&) -> UncurryImpl& = delete;

    template<typename... Args>
    requires(CanUncurry<F&, Args...>)
    constexpr auto operator()(Args&&... args) & -> decltype(auto) {
        return di::apply(m_function, detail::do_tuple_cat(di::forward<Args>(args)...));
    }

    template<typename... Args>
    requires(CanUncurry<F const&, Args...>)
    constexpr auto operator()(Args&&... args) const& -> decltype(auto) {
        return di::apply(m_function, detail::do_tuple_cat(di::forward<Args>(args)...));
    }

    template<typename... Args>
    requires(CanUncurry<F &&, Args...>)
    constexpr auto operator()(Args&&... args) && -> decltype(auto) {
        return di::apply(di::move(m_function), detail::do_tuple_cat(di::forward<Args>(args)...));
    }

    template<typename... Args>
    requires(CanUncurry<F const &&, Args...>)
    constexpr auto operator()(Args&&... args) const&& -> decltype(auto) {
        return di::apply(di::move(m_function), detail::do_tuple_cat(di::forward<Args>(args)...));
    }
};

struct UncurryFunction {
    template<concepts::DecayConstructible F>
    constexpr auto operator()(F&& function) const {
        return UncurryImpl<meta::Decay<F>>(in_place, di::forward<F>(function));
    }
};
}

namespace di::function {
constexpr inline auto uncurry = detail::UncurryFunction {};
}

namespace di {
using function::uncurry;
}

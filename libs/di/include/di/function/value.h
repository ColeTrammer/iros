#pragma once

#include "di/function/pipeable.h"
#include "di/meta/util.h"
#include "di/types/prelude.h"
#include "di/util/forward.h"
#include "di/util/move.h"

namespace di::function {
namespace detail {
    template<typename T>
    struct Value : pipeline::EnablePipeline {
    public:
        template<typename U>
        constexpr explicit Value(InPlace, U&& value) : m_value(util::forward<U>(value)) {}

        constexpr auto operator()(auto&&...) & -> T& { return m_value; }
        constexpr auto operator()(auto&&...) const& -> T const& { return m_value; }
        constexpr auto operator()(auto&&...) && -> T&& { return util::move(m_value); }
        constexpr auto operator()(auto&&...) const&& -> T const&& { return util::move(m_value); }

    private:
        T m_value;
    };

    struct ValueFunction : pipeline::EnablePipeline {
        template<concepts::DecayConstructible T>
        constexpr auto operator()(T&& value) const {
            return Value<meta::Decay<T>> { in_place, util::forward<T>(value) };
        }
    };
}

constexpr inline auto value = detail::ValueFunction {};
}

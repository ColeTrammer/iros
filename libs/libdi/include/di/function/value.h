#pragma once

#include <di/concepts/decay_constructible.h>
#include <di/function/pipeable.h>
#include <di/types/prelude.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename T>
    struct Value : pipeline::EnablePipeline {
    public:
        template<typename U>
        constexpr explicit Value(InPlace, U&& value) : m_value(util::forward<U>(value)) {}

        constexpr T& operator()(auto&&...) & { return m_value; }
        constexpr T const& operator()(auto&&...) const& { return m_value; }
        constexpr T&& operator()(auto&&...) && { return util::move(m_value); }
        constexpr T const&& operator()(auto&&...) const&& { return util::move(m_value); }

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

#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/decay_constructible.h>
#include <di/meta/decay.h>
#include <di/types/prelude.h>
#include <di/vocab/tuple/make_from_tuple.h>
#include <di/vocab/tuple/tuple.h>

namespace di::function {
namespace detail {
    template<typename T, typename... Args>
    class MakeDeferredFunctor {
    public:
        MakeDeferredFunctor() = default;

        MakeDeferredFunctor(MakeDeferredFunctor const&) = default;
        MakeDeferredFunctor(MakeDeferredFunctor&&) = default;

        template<typename... Values>
        requires((concepts::ConstructibleFrom<Args, Values> && ...))
        constexpr explicit MakeDeferredFunctor(InPlace, Values&&... values)
            : m_args(util::forward<Values>(values)...) {}

        MakeDeferredFunctor& operator=(MakeDeferredFunctor const&) = delete;
        MakeDeferredFunctor& operator=(MakeDeferredFunctor&&) = delete;

        constexpr T operator()() &
            requires concepts::ConstructibleFrom<T, Args&...>
        {
            return vocab::make_from_tuple<T>(m_args);
        }

        constexpr T operator()() const&
        requires concepts::ConstructibleFrom<T, Args const&...>
        {
            return vocab::make_from_tuple<T>(m_args);
        }

        constexpr T operator()() &&
            requires concepts::ConstructibleFrom<T, Args&&...>
        {
            return vocab::make_from_tuple<T>(util::move(m_args));
        }

        constexpr T operator()() const&&
        requires concepts::ConstructibleFrom<T, Args const&&...>
        {
            return vocab::make_from_tuple<T>(util::move(m_args));
        }

    private:
        vocab::Tuple<Args...> m_args;
    };

    template<typename T>
    struct MakeDeferredFunction {
        template<concepts::DecayConstructible... Args>
        constexpr auto operator()(Args&&... args) const {
            return MakeDeferredFunctor<T, meta::Decay<Args>...>(in_place, util::forward<Args>(args)...);
        }
    };
}

/// @brief Creates a deferred function object.
///
/// @tparam T The type of the object to be constructed.
/// @param args The arguments to be forwarded to the constructor of the object.
///
/// @returns An invocable which produces a value or type T.
///
/// This function is useful for creating immovable or expensive objects. By
/// only storing the constructor arguments, the object can be constructed at a
/// later time.
template<typename T>
constexpr auto make_deferred = detail::MakeDeferredFunction<T> {};
}

namespace di {
using function::make_deferred;
}

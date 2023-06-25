#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeline.h>
#include <di/util/addressof.h>
#include <di/vocab/optional/get_value.h>
#include <di/vocab/optional/is_nullopt.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/set_nullopt.h>
#include <di/vocab/optional/set_value.h>

namespace di::util {
template<typename T>
class ReferenceWrapper {
private:
    constexpr static T* get_address(T& value) { return util::addressof(value); }

public:
    // Prevent creating reference wrapper's from rvalue (tempory) references.
    constexpr static void get_address(T&&) = delete;

    using Value = T;

    constexpr explicit ReferenceWrapper(vocab::NullOpt) {}

    template<typename U>
    requires(!concepts::ReferenceWrapper<meta::Decay<U>> && requires { get_address(util::declval<U>()); })
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    constexpr ReferenceWrapper(U&& value) : m_pointer(get_address(value)) {}

    constexpr ReferenceWrapper(ReferenceWrapper const&) = default;
    constexpr ReferenceWrapper& operator=(ReferenceWrapper const&) = default;

    constexpr operator T&() const { return get(); }
    constexpr T& get() const { return *m_pointer; }

    template<typename... Args>
    requires(concepts::Invocable<T&, Args...>)
    constexpr meta::InvokeResult<T&, Args...> operator()(Args&&... args) const {
        return function::invoke(get(), util::forward<Args>(args)...);
    }

private:
    // Implement di::vocab::OptionalStorage.
    constexpr friend bool tag_invoke(types::Tag<vocab::is_nullopt>, ReferenceWrapper const& self) {
        return !self.m_pointer;
    }
    constexpr friend T& tag_invoke(types::Tag<vocab::get_value>, ReferenceWrapper const& self) {
        return *self.m_pointer;
    }
    constexpr friend void tag_invoke(types::Tag<vocab::set_nullopt>, ReferenceWrapper& self) {
        self.m_pointer = nullptr;
    }
    constexpr friend void tag_invoke(types::Tag<vocab::set_value>, ReferenceWrapper& self, T& value) {
        self.m_pointer = util::addressof(value);
    }

    T* m_pointer { nullptr };
};

template<typename T>
ReferenceWrapper(T&) -> ReferenceWrapper<T>;

namespace detail {
    struct RefFunction : function::pipeline::EnablePipeline {
        template<typename T>
        constexpr ReferenceWrapper<T> operator()(T& value) const {
            return ReferenceWrapper<T>(value);
        }

        template<typename T>
        constexpr ReferenceWrapper<T> operator()(ReferenceWrapper<T> value) const {
            return ReferenceWrapper<T>(value.get());
        }

        // Prevent construction of reference wrapper's from temporaries.
        template<typename T>
        constexpr void operator()(T const&& value) const = delete;
    };

    struct CRefFunction : function::pipeline::EnablePipeline {
        template<typename T>
        constexpr ReferenceWrapper<T const> operator()(T const& value) const {
            return ReferenceWrapper<T const>(value);
        }

        template<typename T>
        constexpr ReferenceWrapper<T const> operator()(ReferenceWrapper<T> value) const {
            return ReferenceWrapper<T const>(value.get());
        }

        // Prevent construction of reference wrapper's from temporaries.
        template<typename T>
        constexpr void operator()(T const&& value) const = delete;
    };
}

constexpr inline auto ref = detail::RefFunction {};
constexpr inline auto cref = detail::CRefFunction {};
}

namespace di {
using util::cref;
using util::ref;
using util::ReferenceWrapper;
}

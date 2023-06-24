#pragma once

#include <di/assert/assert_bool.h>
#include <di/function/invoke.h>
#include <di/function/monad/monad_interface.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/meta/vocab.h>
#include <di/types/prelude.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::vocab {
template<>
class Optional<void> : public function::monad::MonadInterface<Optional<void>> {
public:
    using Value = void;

    constexpr Optional() = default;
    constexpr Optional(NullOpt) {}

    constexpr Optional(Optional const&) = default;
    constexpr Optional(Optional&&) = default;

    constexpr explicit Optional(InPlace) : m_has_value(true) {}
    constexpr explicit Optional(bool value) : m_has_value(value) {}

    constexpr ~Optional() = default;

    constexpr Optional& operator=(Optional const&) = default;
    constexpr Optional& operator=(Optional&&) = default;

    constexpr Optional& operator=(NullOpt) {
        m_has_value = false;
        return *this;
    }

    constexpr Optional& operator=(bool value) {
        m_has_value = value;
        return *this;
    }

    constexpr bool has_value() const { return m_has_value; }
    constexpr explicit operator bool() const { return has_value(); }

    constexpr void operator*() const { DI_ASSERT(has_value()); }

    constexpr void value() const& { DI_ASSERT(has_value()); }
    constexpr void value() && { DI_ASSERT(has_value()); }

    constexpr void reset() { m_has_value = false; }

    constexpr void emplace() { m_has_value = true; }

private:
    constexpr friend bool operator==(Optional const& a, Optional const& b) { return a.has_value() == b.has_value(); }
    constexpr friend bool operator==(Optional const& a, bool b) { return bool(a) == b; }
    constexpr friend bool operator==(Optional const& a, NullOpt) { return !a.has_value(); }

    constexpr friend strong_ordering operator<=>(Optional const& a, Optional const& b) {
        return a.has_value() <=> b.has_value();
    }
    constexpr friend strong_ordering operator<=>(Optional const& a, bool b) { return bool(a) <=> b; }
    constexpr friend strong_ordering operator<=>(Optional const& a, NullOpt) { return bool(a) <=> false; }

    template<concepts::DecaySameAs<Optional> Self, typename F, typename R = meta::InvokeResult<F>>
    requires(concepts::Optional<R>)
    constexpr friend R tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& f) {
        if (self.has_value()) {
            return function::invoke(util::forward<F>(f));
        } else {
            return R();
        }
    }

    template<concepts::DecaySameAs<Optional> Self, typename F, typename U = meta::UnwrapRefDecay<meta::InvokeResult<F>>>
    constexpr friend Optional<U> tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& f) {
        if (self.has_value()) {
            if constexpr (concepts::LanguageVoid<U>) {
                function::invoke(util::forward<F>(f));
                return Optional<U>(types::in_place);
            } else {
                return Optional<U>(types::in_place, function::invoke(util::forward<F>(f)));
            }
        } else {
            return nullopt;
        }
    }

    template<concepts::DecaySameAs<Optional> Self, concepts::InvocableTo<Optional> F>
    constexpr friend Optional tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&& f) {
        return self.has_value() ? util::forward<Self>(self) : function::invoke(util::forward<F>(f));
    }

    bool m_has_value { false };
};
}

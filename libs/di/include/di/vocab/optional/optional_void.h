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

    constexpr auto operator=(Optional const&) -> Optional& = default;
    constexpr auto operator=(Optional&&) -> Optional& = default;

    constexpr auto operator=(NullOpt) -> Optional& {
        m_has_value = false;
        return *this;
    }

    constexpr auto operator=(bool value) -> Optional& {
        m_has_value = value;
        return *this;
    }

    constexpr auto has_value() const -> bool { return m_has_value; }
    constexpr explicit operator bool() const { return has_value(); }

    constexpr void operator*() const { DI_ASSERT(has_value()); }

    constexpr void value() const& { DI_ASSERT(has_value()); }
    constexpr void value() && { DI_ASSERT(has_value()); }

    constexpr void reset() { m_has_value = false; }

    constexpr void emplace() { m_has_value = true; }

private:
    constexpr friend auto operator==(Optional const& a, Optional const& b) -> bool {
        return a.has_value() == b.has_value();
    }
    constexpr friend auto operator==(Optional const& a, bool b) -> bool { return bool(a) == b; }
    constexpr friend auto operator==(Optional const& a, NullOpt) -> bool { return !a.has_value(); }

    constexpr friend auto operator<=>(Optional const& a, Optional const& b) -> strong_ordering {
        return a.has_value() <=> b.has_value();
    }
    constexpr friend auto operator<=>(Optional const& a, bool b) -> strong_ordering { return bool(a) <=> b; }
    constexpr friend auto operator<=>(Optional const& a, NullOpt) -> strong_ordering { return bool(a) <=> false; }

    template<concepts::DecaySameAs<Optional> Self, typename F, typename R = meta::InvokeResult<F>>
    requires(concepts::Optional<R>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& f) -> R {
        if (self.has_value()) {
            return function::invoke(util::forward<F>(f));
        }
        return R();
    }

    template<concepts::DecaySameAs<Optional> Self, typename F, typename U = meta::UnwrapRefDecay<meta::InvokeResult<F>>>
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& f) -> Optional<U> {
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
    constexpr friend auto tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&& f) -> Optional {
        return self.has_value() ? util::forward<Self>(self) : function::invoke(util::forward<F>(f));
    }

    bool m_has_value { false };
};
}

#pragma once

#include <di/container/prelude.h>
#include <di/container/string/prelude.h>
#include <di/container/string/string_view.h>
#include <di/function/prelude.h>
#include <di/meta/language.h>
#include <di/parser/prelude.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/prelude.h>

namespace di::cli::detail {
class Option {
private:
    using Parse = Result<> (*)(void*, Optional<TransparentStringView>);

    template<auto member>
    constexpr static auto concrete_parse(void* output_untyped, Optional<TransparentStringView> input) -> Result<> {
        using Base = meta::MemberPointerClass<decltype(member)>;
        using Value = meta::MemberPointerValue<decltype(member)>;

        auto output = static_cast<Base*>(output_untyped);
        if constexpr (concepts::SameAs<Value, bool>) {
            DI_ASSERT(!input);
            (*output).*member = true;
            return {};
        } else {
            DI_ASSERT(input);
            if constexpr (concepts::Optional<Value>) {
                (*output).*member = DI_TRY(parser::parse<meta::OptionalValue<Value>>(*input));
            } else {
                (*output).*member = DI_TRY(parser::parse<Value>(*input));
            }
            return {};
        }
    }

public:
    Option() = default;

    template<auto member>
    constexpr explicit Option(Constexpr<member>, Optional<char> short_name = {},
                              Optional<TransparentStringView> long_name = {}, StringView description = {},
                              bool required = false, bool always_succeeds = false)
        : m_parse(concrete_parse<member>)
        , m_short_name(short_name)
        , m_long_name(long_name)
        , m_description(description)
        , m_required(required)
        , m_always_succeeds(always_succeeds)
        , m_boolean(di::SameAs<meta::MemberPointerValue<decltype(member)>, bool>) {}

    constexpr auto parse(void* base, Optional<TransparentStringView> input) const {
        DI_ASSERT(m_parse);
        return m_parse(base, input);
    }
    constexpr auto short_name() const { return m_short_name; }
    constexpr auto long_name() const { return m_long_name; }
    constexpr auto description() const { return m_description; }
    constexpr auto required() const { return m_required; }
    constexpr auto boolean() const { return m_boolean; }
    constexpr auto always_succeeds() const { return m_always_succeeds; }

    constexpr auto long_display_name() const {
        auto result = di::TransparentString {};
        result += di::single('-');
        result += di::single('-');
        result += m_long_name.value();
        return result;
    }

    constexpr auto short_display_name() const {
        auto result = di::TransparentString {};
        result += di::single('-');
        result += di::single(m_short_name.value());
        return result;
    }

    constexpr auto display_name() const {
        if (long_name()) {
            return long_display_name();
        }
        if (short_name()) {
            return short_display_name();
        }
        return di::TransparentString {};
    }

private:
    Parse m_parse { nullptr };
    Optional<char> m_short_name;
    Optional<TransparentStringView> m_long_name;
    StringView m_description;
    bool m_required { false };
    bool m_always_succeeds { false };
    bool m_boolean { false };
};
}

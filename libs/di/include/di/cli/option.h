#pragma once

#include <di/container/prelude.h>
#include <di/container/string/prelude.h>
#include <di/function/prelude.h>
#include <di/meta/language.h>
#include <di/parser/prelude.h>
#include <di/vocab/prelude.h>

namespace di::cli::detail {
template<auto member>
class Option {
private:
    using Base = meta::MemberPointerClass<decltype(member)>;
    using Value = meta::MemberPointerValue<decltype(member)>;

public:
    constexpr explicit Option(Optional<char> short_name = {}, Optional<TransparentStringView> long_name = {},
                              Optional<StringView> description = {}, bool required = false)
        : m_short_name(short_name), m_long_name(long_name), m_description(description), m_required(required) {}

    constexpr Result<void> parse(Base* output, Optional<TransparentStringView> input) const {
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

    constexpr static bool boolean() { return concepts::SameAs<Value, bool>; }

    constexpr auto short_name() const { return m_short_name; }
    constexpr auto long_name() const { return m_long_name; }
    constexpr auto description() const { return m_description; }
    constexpr auto required() const { return m_required; }

private:
    Optional<char> m_short_name;
    Optional<TransparentStringView> m_long_name;
    Optional<StringView> m_description;
    bool m_required { false };
};
}

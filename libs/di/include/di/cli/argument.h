#pragma once

#include <di/container/string/encoding.h>
#include <di/container/string/prelude.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/prelude.h>
#include <di/parser/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::cli::detail {
template<auto member>
class Argument {
private:
    static_assert(concepts::MemberObjectPointer<decltype(member)>,
                  "Argument member must be a pointer to a member object.");

    using Base = meta::MemberPointerClass<decltype(member)>;
    using Value = meta::MemberPointerValue<decltype(member)>;

public:
    constexpr explicit Argument(Optional<StringView> argument_name = {}, Optional<StringView> description = {},
                                bool required = false)
        : m_argument_name(argument_name), m_description(description), m_required(required) {}

    constexpr Result<void> parse(Base* output, Span<TransparentStringView> input) const {
        if constexpr (variadic()) {
            auto& vector = (*output).*member = Value();
            for (auto view : input) {
                if constexpr (concepts::Expected<meta::detail::VectorAllocResult<Value>>) {
                    DI_TRY(vector.emplace_back(DI_TRY(parser::parse<meta::ContainerValue<Value>>(view))));
                } else {
                    vector.emplace_back(DI_TRY(parser::parse<meta::ContainerValue<Value>>(view)));
                }
            }
            return {};
        } else {
            DI_ASSERT(input.size() == 1);
            if constexpr (concepts::Optional<Value>) {
                (*output).*member = DI_TRY(parser::parse<meta::OptionalValue<Value>>(input[0]));
            } else {
                (*output).*member = DI_TRY(parser::parse<Value>(input[0]));
            }
            return {};
        }
    }

    constexpr static bool variadic() {
        // Match any form of vector, but not a string-like type.
        return concepts::detail::MutableVector<Value> && !concepts::HasEncoding<Value>;
    }

    constexpr usize required_argument_count() const { return required() ? 1 : 0; }

    constexpr auto argument_name() const { return m_argument_name; }
    constexpr auto description() const { return m_description; }
    constexpr auto required() const { return m_required; }

private:
    Optional<StringView> m_argument_name;
    Optional<StringView> m_description;
    bool m_required { false };
};
}

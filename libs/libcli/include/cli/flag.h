#pragma once

#include <cli/error.h>
#include <liim/function.h>
#include <liim/option.h>
#include <liim/result.h>
#include <liim/string_view.h>
#include <liim/try.h>

namespace Cli {
template<typename T>
struct IsOption {
    using Type = T;
    static constexpr bool value = false;
};

template<typename T>
struct IsOption<Option<T>> {
    using Type = T;
    static constexpr bool value = true;
};

class Flag {
private:
    using ParserCallback = Result<Monostate, Error> (*)(Option<StringView>, void*);

    template<auto member>
    class FlagBuilder {};

    template<typename StructType, typename ValueType, ValueType StructType::*member>
    class FlagBuilder<member> {
    private:
        static constexpr bool is_option = IsOption<ValueType>::value;
        using ParserType = IsOption<ValueType>::Type;

    public:
        constexpr static FlagBuilder boolean() requires(LIIM::IsSame<ValueType, bool>::value) {
            return FlagBuilder(
                [](auto, void* output_ptr) -> Result<Monostate, Error> {
                    StructType& output = *static_cast<StructType*>(output_ptr);
                    output.*member = true;
                    return Ok(Monostate {});
                },
                false);
        }

        constexpr static FlagBuilder value() requires(is_option) {
            return FlagBuilder(
                [](auto input, void* output_ptr) -> Result<Monostate, Error> {
                    StructType& output = *static_cast<StructType*>(output_ptr);
                    auto value = TRY(Ext::parse<ParserType>(*input));
                    output.*member = move(value);
                    return Ok(Monostate {});
                },
                true);
        }

        constexpr FlagBuilder(ParserCallback parser, bool requires_value) : m_parser(parser), m_requires_value(requires_value) {}

        constexpr FlagBuilder& short_name(char name) {
            m_short_name = name;
            return *this;
        }

        constexpr FlagBuilder& long_name(StringView name) {
            m_long_name = name;
            return *this;
        }

        constexpr FlagBuilder& description(StringView description) {
            m_description = description;
            return *this;
        }

        constexpr Flag flag() const { return Flag(m_parser, move(m_short_name), move(m_long_name), move(m_description), m_requires_value); }

        constexpr operator Flag() const { return flag(); }

    private:
        ParserCallback m_parser;
        Option<char> m_short_name;
        Option<StringView> m_long_name;
        Option<StringView> m_description;
        bool m_requires_value { false };
    };

public:
    template<auto member>
    static constexpr FlagBuilder<member> boolean() {
        return FlagBuilder<member>::boolean();
    }

    template<auto member>
    static constexpr FlagBuilder<member> value() {
        return FlagBuilder<member>::value();
    }

    constexpr Flag() = default;

    constexpr Option<char> short_name() const { return m_short_name; }
    constexpr Option<StringView> long_name() const { return m_long_name; }
    constexpr Option<StringView> description() const { return m_description; }

    constexpr bool requires_value() const { return m_requires_value; }

    constexpr Result<Monostate, Error> validate(Option<StringView> value, void* output) const { return m_parser(move(value), output); }

private:
    constexpr Flag(ParserCallback parser, Option<char> short_name, Option<StringView> long_name, Option<StringView> description,
                   bool requires_value)
        : m_parser(parser)
        , m_short_name(move(short_name))
        , m_long_name(move(long_name))
        , m_description(move(description))
        , m_requires_value(requires_value) {}

    ParserCallback m_parser;
    Option<char> m_short_name;
    Option<StringView> m_long_name;
    Option<StringView> m_description;
    bool m_requires_value { false };
};
}

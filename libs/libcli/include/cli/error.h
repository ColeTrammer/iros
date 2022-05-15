#pragma once

#include <cli/forward.h>
#include <liim/format.h>
#include <liim/string.h>
#include <liim/variant.h>

namespace Cli {
class UnexpectedShortFlag {
public:
    explicit UnexpectedShortFlag(char flag) : m_flag(flag) {}

    String to_message() const { return format("Encountered unexpected flag `-{}'", String { m_flag }); }

private:
    char m_flag;
};

class UnexpectedLongFlag {
public:
    explicit UnexpectedLongFlag(StringView flag) : m_flag(flag) {}

    String to_message() const { return format("Encountered unexpected flag `--{}'", m_flag); }

private:
    StringView m_flag;
};

class LongFlagRequiresValue {
public:
    explicit LongFlagRequiresValue(StringView flag) : m_flag(flag) {}

    String to_message() const { return format("Flag `--{}' requires value, but none provided", m_flag); }

private:
    StringView m_flag;
};

class ShortFlagRequiresValue {
public:
    explicit ShortFlagRequiresValue(char flag) : m_flag(flag) {}

    String to_message() const { return format("Flag `-{}' requires value, but none provided", String { m_flag }); }

private:
    char m_flag;
};

class UnexpectedPositionalArgument {
public:
    explicit UnexpectedPositionalArgument(StringView value) : m_value(value) {}

    String to_message() const { return format("Encountered unexpected positional argument `{}'", m_value); }

private:
    StringView m_value;
};

class MissingPositionalArgument {
public:
    explicit MissingPositionalArgument(StringView name) : m_name(name) {}

    String to_message() const { return format("Positional argument `{}' requires value, but none provided", m_name); }

private:
    StringView m_name;
};

using Error = Variant<UnexpectedShortFlag, UnexpectedLongFlag, ShortFlagRequiresValue, LongFlagRequiresValue, UnexpectedPositionalArgument,
                      MissingPositionalArgument>;
}

namespace LIIM::Format {
template<>
struct Formatter<Cli::Error> : Formatter<StringView> {
    void format(const Cli::Error& value, FormatContext& context) {
        auto message = value.visit([](auto&& error) {
            return error.to_message();
        });
        return format_string_view(message.view(), context);
    }
};
}

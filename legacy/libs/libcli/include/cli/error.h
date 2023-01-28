#pragma once

#include <cli/forward.h>
#include <ext/error.h>
#include <ext/parser.h>
#include <liim/format.h>
#include <liim/string.h>
#include <liim/variant.h>

namespace Cli {
class UnexpectedShortFlag {
public:
    explicit UnexpectedShortFlag(char flag) : m_flag(flag) {}

    String to_message() const { return format("Encountered unexpected flag `-{}'", String { m_flag }); }

    char flag() const { return m_flag; }

private:
    char m_flag;
};

class UnexpectedLongFlag {
public:
    explicit UnexpectedLongFlag(StringView flag) : m_flag(flag) {}

    String to_message() const { return format("Encountered unexpected flag `--{}'", m_flag); }

    StringView flag() const { return m_flag; }

private:
    StringView m_flag;
};

class LongFlagRequiresValue {
public:
    explicit LongFlagRequiresValue(StringView flag) : m_flag(flag) {}

    String to_message() const { return format("Flag `--{}' requires value, but none provided", m_flag); }

    StringView flag() const { return m_flag; }

private:
    StringView m_flag;
};

class ShortFlagRequiresValue {
public:
    explicit ShortFlagRequiresValue(char flag) : m_flag(flag) {}

    String to_message() const { return format("Flag `-{}' requires value, but none provided", String { m_flag }); }

    char flag() const { return m_flag; }

private:
    char m_flag;
};

class MissingRequiredFlag {
public:
    explicit MissingRequiredFlag(Option<char> short_name, Option<StringView> long_name)
        : m_short_name(short_name), m_long_name(long_name) {}

    String to_message() const {
        auto flag_name = [&] {
            if (m_short_name && m_long_name) {
                return format("-{}/--{}", *m_short_name, *m_long_name);
            } else if (m_short_name) {
                return format("-{}", *m_short_name);
            } else if (m_long_name) {
                return format("--{}", *m_long_name);
            } else {
                return format("{}", "[Invalid Flag]");
            }
        }();

        return format("Flag `{}' is required, but was not provided", flag_name);
    }

    Option<char> short_name() const { return m_short_name; }
    Option<StringView> long_name() const { return m_long_name; }

private:
    Option<char> m_short_name;
    Option<StringView> m_long_name;
};

class UnexpectedPositionalArgument {
public:
    explicit UnexpectedPositionalArgument(StringView value) : m_value(value) {}

    String to_message() const { return format("Encountered unexpected positional argument `{}'", m_value); }

    StringView value() const { return m_value; }

private:
    StringView m_value;
};

class MissingPositionalArgument {
public:
    explicit MissingPositionalArgument(StringView argument_name) : m_argument_name(argument_name) {}

    String to_message() const { return format("Positional argument `{}' requires value, but none provided", m_argument_name); }

    StringView argument_name() const { return m_argument_name; }

private:
    StringView m_argument_name;
};

class EmptyPositionalArgumentList {
public:
    explicit EmptyPositionalArgumentList(StringView argument_name) : m_argument_name(argument_name) {}

    String to_message() const { return format("Positional argument list `{}' requires at least one value", m_argument_name); }

    StringView argument_name() const { return m_argument_name; }

private:
    StringView m_argument_name;
};

using Error = Variant<Ext::ParserError, UnexpectedShortFlag, UnexpectedLongFlag, ShortFlagRequiresValue, LongFlagRequiresValue,
                      MissingRequiredFlag, UnexpectedPositionalArgument, MissingPositionalArgument, EmptyPositionalArgumentList>;
}

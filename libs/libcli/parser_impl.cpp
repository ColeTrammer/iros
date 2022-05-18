#include <cli/argument.h>
#include <cli/flag.h>
#include <cli/parser_impl.h>
#include <liim/result.h>
#include <liim/span.h>
#include <liim/string_view.h>
#include <liim/try.h>

namespace Cli::Detail {
ParserImpl::ParserImpl(Span<const Flag> flags, Span<const Argument> arguments, bool help_flag, bool version_flag)
    : m_flags(flags), m_arguments(arguments), m_help_flag(help_flag), m_version_flag(version_flag) {}

ParserImpl::~ParserImpl() {}

Result<const Flag&, UnexpectedLongFlag> ParserImpl::lookup_long_flag(StringView name) const {
    for (auto& flag : m_flags) {
        if (flag.long_name() == name) {
            return Ok<const Flag&>(flag);
        }
    }
    return Err(UnexpectedLongFlag(name));
}

Result<const Flag&, UnexpectedShortFlag> ParserImpl::lookup_short_flag(char name) const {
    for (auto& flag : m_flags) {
        if (flag.short_name() == name) {
            return Ok<const Flag&>(flag);
        }
    }
    return Err(UnexpectedShortFlag(name));
}

Result<Monostate, Error> ParserImpl::parse(Span<StringView> input, void* output) const {
    constexpr auto position_argument_only_marker = "--"sv;
    constexpr auto long_flag_prefix = "--"sv;
    constexpr auto long_flag_value_marker = '=';
    constexpr auto short_flag_prefix = '-';

    assert(!input.empty());

    // Skip the program name
    input = input.subspan(1);

    Vector<StringView> positional_arguments;

    // Parse flags denoted by the long or short flag prefixes
    // and store anything else in the positional argument vector.
    for (size_t i = 0; i < input.size(); i++) {
        auto item = input[i];

        // Skip the remaining input if we recieve the positional argument marker ('--').
        if (item == position_argument_only_marker) {
            while (++i < input.size()) {
                positional_arguments.add(input[i]);
            }
            break;
        }

        // Handle long flags.
        if (item.starts_with(long_flag_prefix)) {
            // Skip the flag prefix.
            item = item.substring(long_flag_prefix.size());

            auto value_marker = item.index_of(long_flag_value_marker);
            auto flag_name = value_marker ? item.substring(0, *value_marker) : item;
            auto& flag = TRY(lookup_long_flag(flag_name));

            auto value = [&]() -> Option<StringView> {
                // If there was a value marker, the value is directly after it.
                // Otherwise, if the flag requires a value, look for it in
                // the next item of input, and consume this one.
                if (value_marker) {
                    return item.substring(*value_marker + 1);
                }
                if (!flag.requires_value()) {
                    return None {};
                }
                return input.get(++i);
            }();
            if (!value && flag.requires_value()) {
                return Err(LongFlagRequiresValue(flag_name));
            }

            TRY(flag.validate(value, output));
            continue;
        }

        // Handle short flags.
        if (!item.empty() && item.first() == short_flag_prefix) {
            // Skip the flag prefix.
            item = item.substring(1);

            // Iterate over all short flags present.
            for (size_t j = 0; j < item.size(); j++) {
                auto flag_name = item[j];
                auto& flag = TRY(lookup_short_flag(flag_name));

                auto value = [&]() -> Option<StringView> {
                    if (!flag.requires_value()) {
                        return None {};
                    }

                    // Skip this flag character, and see if anything is left over.
                    auto possible_value = item.substring(j + 1);
                    if (!possible_value.empty()) {
                        return possible_value;
                    }

                    // Look to the next argument, consuming this one.
                    return input.get(++i);
                }();
                if (!value && flag.requires_value()) {
                    return Err(ShortFlagRequiresValue(flag_name));
                }

                TRY(flag.validate(value, output));

                // If a value was encountered, there are no short options left to parse in this item.
                if (value) {
                    break;
                }
            }
            continue;
        }

        // This is a positional argument, just add it to be processed later.
        positional_arguments.add(item);
    }

    // Parse the positional arguments
    if (m_arguments.size() == 0) {
        return Ok(Monostate {});
    }

    // Variable arguments come first.
    if (m_arguments.first(1)[0].is_list()) {
        if (static_cast<size_t>(positional_arguments.size()) < m_arguments.size() - 1) {
            return Err(MissingPositionalArgument(m_arguments[positional_arguments.size()].name()));
        }

        auto variable_arguments = Vector<StringView> {};
        for (size_t i = 0; i < positional_arguments.size() - m_arguments.size() + 1; i++) {
            variable_arguments.add(positional_arguments[i]);
        }
        TRY(m_arguments.first(1)[0].validate(variable_arguments.span(), output));
        for (size_t i = 1; i < m_arguments.size(); i++) {
            TRY(m_arguments[i].validate(positional_arguments[variable_arguments.size() + i - 1], output));
        }
        return Ok(Monostate {});
    }

    // Variables arguments come last.
    if (m_arguments.last(1)[0].is_list()) {
        if (static_cast<size_t>(positional_arguments.size()) < m_arguments.size() - 1) {
            return Err(MissingPositionalArgument(m_arguments[positional_arguments.size() - 1].name()));
        }

        for (size_t i = 0; i < m_arguments.size() - 1; i++) {
            TRY(m_arguments[i].validate(positional_arguments[i], output));
        }
        auto variable_arguments = Vector<StringView> {};
        for (size_t i = m_arguments.size() - 1; i < static_cast<size_t>(positional_arguments.size()); i++) {
            variable_arguments.add(positional_arguments[i]);
        }
        TRY(m_arguments.last(1)[0].validate(variable_arguments.span(), output));
        return Ok(Monostate {});
    }

    // No argument lists, this means that optional positional arguments are allowed.
    if (m_arguments.size() < static_cast<size_t>(positional_arguments.size())) {
        return Err(UnexpectedPositionalArgument(positional_arguments[m_arguments.size()]));
    }

    size_t required_argument_count = 0;
    for (auto& argument : m_arguments) {
        if (!argument.is_optional()) {
            required_argument_count++;
        }
    }
    if (static_cast<size_t>(positional_arguments.size()) < required_argument_count) {
        size_t required_argument_index = 0;
        for (auto& argument : m_arguments) {
            if (!argument.is_optional()) {
                if (required_argument_index++ == static_cast<size_t>(positional_arguments.size())) {
                    return Err(MissingPositionalArgument(argument.name()));
                }
            }
        }
    }

    int input_positional_index = 0;
    size_t argument_index = 0;
    for (; argument_index < m_arguments.size() && input_positional_index < positional_arguments.size(); argument_index++) {
        auto remaining_arguments = static_cast<size_t>(positional_arguments.size() - input_positional_index);
        auto& argument = m_arguments[argument_index];
        if (argument.is_optional() && remaining_arguments < required_argument_count) {
            continue;
        }
        auto item = positional_arguments[input_positional_index];
        TRY(argument.validate(item, output));
        if (!argument.is_optional()) {
            required_argument_count--;
        }
        input_positional_index++;
    }

    return Ok(Monostate {});
}
}

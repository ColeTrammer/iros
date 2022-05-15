#include <cli/argument.h>
#include <cli/flag.h>
#include <cli/parser_impl.h>
#include <liim/result.h>
#include <liim/span.h>
#include <liim/string_view.h>
#include <liim/try.h>

namespace Cli::Detail {
ParserImpl::ParserImpl(Span<const Flag> flags, Span<const Argument> arguments) : m_flags(flags), m_arguments(arguments) {}

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
    int input_positional_index = 0;
    size_t argument_index = 0;
    for (; argument_index < m_arguments.size() && input_positional_index < positional_arguments.size();
         argument_index++, input_positional_index++) {
        auto item = positional_arguments[input_positional_index];
        auto& argument = m_arguments[argument_index];
        TRY(argument.validate(item, output));
    }

    // Return errors if there are either too many or missing positional arguments.
    if (input_positional_index < positional_arguments.size()) {
        return Err(UnexpectedPositionalArgument(positional_arguments[input_positional_index]));
    }
    if (argument_index < m_arguments.size()) {
        return Err(MissingPositionalArgument(m_arguments[argument_index].name()));
    }

    return Ok(Monostate {});
}
}

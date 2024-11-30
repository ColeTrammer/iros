#pragma once

#include <di/any/concepts/impl.h>
#include <di/cli/argument.h>
#include <di/cli/option.h>
#include <di/container/algorithm/any_of.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/algorithm/rotate.h>
#include <di/container/algorithm/sum.h>
#include <di/container/string/string_view.h>
#include <di/container/string/utf8_encoding.h>
#include <di/container/vector/static_vector.h>
#include <di/format/style.h>
#include <di/function/monad/monad_try.h>
#include <di/function/not_fn.h>
#include <di/function/prelude.h>
#include <di/io/interface/writer.h>
#include <di/io/string_writer.h>
#include <di/io/writer_print.h>
#include <di/io/writer_println.h>
#include <di/meta/constexpr.h>
#include <di/meta/language.h>
#include <di/vocab/optional/lift_bool.h>

namespace di::cli {
namespace detail {
    template<concepts::Object Base>
    class Parser {
    private:
        constexpr static auto max_options = 100ZU;
        constexpr static auto max_arguments = 100ZU;

    public:
        constexpr explicit Parser(StringView app_name, StringView description)
            : m_app_name(app_name), m_description(description) {}

        template<auto member>
        requires(concepts::MemberObjectPointer<decltype(member)> &&
                 concepts::SameAs<Base, meta::MemberPointerClass<decltype(member)>>)
        constexpr auto option(Optional<char> short_name, Optional<TransparentStringView> long_name,
                              StringView description, bool required = false, bool always_succeed = false) && {
            auto new_option = Option { c_<member>, short_name, long_name, description, required, always_succeed };
            DI_ASSERT(m_options.push_back(new_option));
            return di::move(*this);
        }

        constexpr auto help(Optional<char> short_name = {}, Optional<TransparentStringView> long_name = "help"_tsv,
                            StringView description = "Print help message"_sv) {
            static_assert(
                requires { c_<&Base::help>; },
                "A help message requires the argument type to have a boolean help member.");
            static_assert(SameAs<meta::MemberPointerValue<decltype(&Base::help)>, bool>,
                          "A help message requires the argument type to have a boolean help member.");
            return di::move(*this).template option<&Base::help>(short_name, long_name, description, false, true);
        }

        template<auto member>
        requires(concepts::MemberObjectPointer<decltype(member)> &&
                 concepts::SameAs<Base, meta::MemberPointerClass<decltype(member)>>)
        constexpr auto argument(StringView name, StringView description, bool required = false) && {
            auto new_argument = Argument { c_<member>, name, description, required };
            DI_ASSERT(m_arguments.push_back(new_argument));
            return di::move(*this);
        }

        // NOLINTNEXTLINE(readability-function-cognitive-complexity)
        constexpr auto parse(Span<TransparentStringView> args) -> Result<Base> {
            using namespace di::string_literals;

            if (args.empty()) {
                return Unexpected(BasicError::InvalidArgument);
            }
            args = *args.subspan(1);

            auto seen_arguments = Array<bool, max_options> {};
            seen_arguments.fill(false);

            auto count_option_processed = usize { 0 };

            auto send_arg_to_back = [&](usize i) {
                container::rotate(*args.subspan(i), args.begin() + i + 1);
                count_option_processed++;
            };

            auto result = Base {};
            for (usize i = 0; i < args.size(); i++) {
                auto arg_index = i - count_option_processed;
                auto arg = args[arg_index];

                // Handle positional argument.
                if (!arg.starts_with('-')) {
                    continue;
                }

                // Handle exactly "--".
                if (arg == "--"_tsv) {
                    send_arg_to_back(arg_index);
                    break;
                }

                // Handle short argument.
                if (!arg.starts_with("--"_tsv)) {
                    for (usize char_index = 1; char_index < arg.size(); char_index++) {
                        auto index = lookup_short_name(arg[char_index]);
                        if (!index) {
                            return Unexpected(BasicError::InvalidArgument);
                        }

                        // Parse boolean flag.
                        if (option_boolean(*index)) {
                            DI_TRY(option_parse(*index, seen_arguments, &result, {}));
                            if (option_always_succeeds(*index)) {
                                return result;
                            }
                            continue;
                        }

                        // Parse short option with directly specified value: '-iinput.txt'
                        auto value_view = arg.substr(arg.begin() + isize(char_index + 1));
                        if (!value_view.empty()) {
                            DI_TRY(option_parse(*index, seen_arguments, &result, value_view));
                            if (option_always_succeeds(*index)) {
                                return result;
                            }
                            break;
                        }

                        // Fail if the is no subsequent arguments left.
                        if (i + 1 >= args.size()) {
                            return Unexpected(BasicError::InvalidArgument);
                        }

                        // Use the next argument as the value.
                        DI_TRY(option_parse(*index, seen_arguments, &result, args[arg_index + 1]));
                        if (option_always_succeeds(*index)) {
                            return result;
                        }
                        send_arg_to_back(arg_index);
                        i++;
                    }
                    send_arg_to_back(arg_index);
                    continue;
                }

                // Handle long arguments.
                auto equal = arg.find('=');
                auto name = ""_tsv;
                if (!equal) {
                    name = arg.substr(arg.begin() + 2);
                } else {
                    name = arg.substr(arg.begin() + 2, equal.begin());
                }

                auto index = lookup_long_name(name);
                if (!index) {
                    return Unexpected(BasicError::InvalidArgument);
                }

                if (option_boolean(*index)) {
                    if (equal) {
                        return Unexpected(BasicError::InvalidArgument);
                    }
                    DI_TRY(option_parse(*index, seen_arguments, &result, {}));
                    if (option_always_succeeds(*index)) {
                        return result;
                    }
                    send_arg_to_back(arg_index);
                    continue;
                }

                auto value = ""_tsv;
                if (!equal && i + 1 >= args.size()) {
                    return Unexpected(BasicError::InvalidArgument);
                }
                if (!equal) {
                    value = args[arg_index + 1];
                    send_arg_to_back(arg_index);
                    send_arg_to_back(arg_index);
                    i++;
                } else {
                    value = arg.substr(equal.end());
                    send_arg_to_back(arg_index);
                }

                DI_TRY(option_parse(*index, seen_arguments, &result, value));
                if (option_always_succeeds(*index)) {
                    return result;
                }
            }

            // Validate all required arguments were processed.
            for (usize i = 0; i < m_options.size(); i++) {
                if (!seen_arguments[i] && option_required(i)) {
                    return Unexpected(BasicError::InvalidArgument);
                }
            }

            // All the positional arguments are now at the front of the array.
            auto positional_arguments = *args.subspan(0, args.size() - count_option_processed);
            if (positional_arguments.size() < minimum_required_argument_count()) {
                return Unexpected(BasicError::InvalidArgument);
            }

            auto argument_index = usize(0);
            for (auto i = usize(0); i < positional_arguments.size(); argument_index++) {
                auto count_to_consume = !argument_variadic(i) ? 1 : positional_arguments.size() - argument_count() + 1;
                auto input = *positional_arguments.subspan(i, count_to_consume);
                DI_TRY(argument_parse(argument_index, &result, input));
                i += count_to_consume;
            }
            return result;
        }

        template<Impl<io::Writer> Writer>
        constexpr void write_help(Writer& writer) const {
            using Enc = container::string::Utf8Encoding;

            constexpr auto header_effect = di::FormatEffect::Bold | di::FormatColor::Yellow;
            constexpr auto program_effect = di::FormatEffect::Bold;
            constexpr auto option_effect = di::FormatColor::Cyan;
            constexpr auto option_value_effect = di::FormatColor::Green;
            constexpr auto argument_effect = di::FormatColor::Green;

            io::writer_println<Enc>(writer, "{}"_sv, di::Styled("NAME:"_sv, header_effect));
            io::writer_println<Enc>(writer, "  {}: {}"_sv, di::Styled(m_app_name, program_effect), m_description);

            io::writer_println<Enc>(writer, "\n{}"_sv, di::Styled("USAGE:"_sv, header_effect));
            io::writer_print<Enc>(writer, "  {}"_sv, di::Styled(m_app_name, program_effect));
            if (di::any_of(m_options, di::not_fn(&Option::required))) {
                io::writer_print<Enc>(writer, " [OPTIONS]"_sv);
            }
            for (auto const& option : m_options) {
                if (option.required()) {
                    io::writer_print<Enc>(writer, " {}"_sv, di::Styled(option.display_name(), option_effect));

                    if (!option.boolean()) {
                        io::writer_print<Enc>(writer, " {}"_sv, di::Styled("<VALUE>"_sv, option_value_effect));
                    }
                }
            }
            for (auto const& argument : m_arguments) {
                io::writer_print<Enc>(writer, " {}"_sv, di::Styled(argument.display_name(), argument_effect));
            }
            io::writer_println<Enc>(writer, ""_sv);

            if (!m_arguments.empty()) {
                io::writer_println<Enc>(writer, "\n{}"_sv, di::Styled("ARGUMENTS:"_sv, header_effect));
                for (auto const& argument : m_arguments) {
                    io::writer_println<Enc>(writer, "  {}: {}"_sv, di::Styled(argument.display_name(), argument_effect),
                                            argument.description());
                }
            }

            if (!m_options.empty()) {
                io::writer_println<Enc>(writer, "\n{}"_sv, di::Styled("OPTIONS:"_sv, header_effect));
                for (auto const& option : m_options) {
                    io::writer_print<Enc>(writer, "  "_sv);
                    if (option.short_name()) {
                        io::writer_print<Enc>(writer, "{}"_sv, di::Styled(option.short_display_name(), option_effect));
                        if (!option.boolean() && !option.long_name()) {
                            io::writer_print<Enc>(writer, " {}"_sv, di::Styled("<VALUE>"_sv, option_value_effect));
                        }
                    }
                    if (option.short_name() && option.long_name()) {
                        io::writer_print<Enc>(writer, ", "_sv);
                    }
                    if (option.long_name()) {
                        io::writer_print<Enc>(writer, "{}"_sv, di::Styled(option.long_display_name(), option_effect));
                        if (!option.boolean()) {
                            io::writer_print<Enc>(writer, " {}"_sv, di::Styled("<VALUE>"_sv, option_value_effect));
                        }
                    }
                    io::writer_println<Enc>(writer, ": {}"_sv, option.description());
                }
            }
        }

        constexpr auto help_string() const {
            auto writer = di::StringWriter {};
            write_help(writer);
            return di::move(writer).output();
        }

    private:
        constexpr auto option_required(usize index) const -> bool { return m_options[index].required(); }
        constexpr auto option_boolean(usize index) const -> bool { return m_options[index].boolean(); }
        constexpr auto option_always_succeeds(usize index) const -> bool { return m_options[index].always_succeeds(); }

        constexpr auto option_parse(usize index, Span<bool> seen_arguments, Base* output,
                                    Optional<TransparentStringView> input) const -> Result<void> {
            DI_TRY(m_options[index].parse(output, input));
            seen_arguments[index] = true;
            return {};
        }

        constexpr auto argument_variadic(usize index) const -> bool { return m_arguments[index].variadic(); }

        constexpr auto argument_parse(usize index, Base* output, Span<TransparentStringView> input) const
            -> Result<void> {
            return m_arguments[index].parse(output, input);
        }

        constexpr auto minimum_required_argument_count() const -> usize {
            return di::sum(m_arguments | di::transform(&Argument::required_argument_count));
        }

        constexpr auto argument_count() const -> usize { return m_arguments.size(); }

        constexpr auto lookup_short_name(char short_name) const -> Optional<usize> {
            auto const* it = di::find(m_options, short_name, &Option::short_name);
            return lift_bool(it != m_options.end()) % [&] {
                return usize(it - m_options.begin());
            };
        }

        constexpr auto lookup_long_name(TransparentStringView long_name) const -> Optional<usize> {
            auto const* it = di::find(m_options, long_name, &Option::long_name);
            return lift_bool(it != m_options.end()) % [&] {
                return usize(it - m_options.begin());
            };
        }

        StringView m_app_name;
        StringView m_description;
        StaticVector<Option, Constexpr<max_options>> m_options;
        StaticVector<Argument, Constexpr<max_arguments>> m_arguments;
    };
}

template<concepts::Object T>
constexpr auto cli_parser(StringView app_name, StringView description) {
    return detail::Parser<T> { app_name, description };
}
}

namespace di {
using cli::cli_parser;
}

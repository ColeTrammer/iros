#pragma once

#include <di/cli/argument.h>
#include <di/cli/option.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/algorithm/rotate.h>
#include <di/container/string/string_view.h>
#include <di/function/prelude.h>

namespace di::cli {
namespace detail {
    template<typename Base, typename Options, typename Arguments>
    class Parser;

    template<concepts::Object Base, typename... Options, typename... Arguments>
    class Parser<Base, meta::List<Options...>, meta::List<Arguments...>> {
    private:
    public:
        constexpr explicit Parser(Optional<StringView> app_name, Optional<StringView> description,
                                  Tuple<Options...> options, Tuple<Arguments...> arguments)
            : m_app_name(app_name), m_description(description), m_options(options), m_arguments(arguments) {}

        template<auto member>
        requires(concepts::MemberObjectPointer<decltype(member)> &&
                 concepts::SameAs<Base, meta::MemberPointerClass<decltype(member)>>)
        constexpr auto flag(Optional<char> short_name, Optional<TransparentStringView> long_name = {},
                            Optional<StringView> description = {}, bool required = false) {
            auto new_option = Option<member> { short_name, long_name, description, required };
            return Parser<Base, meta::List<Options..., decltype(new_option)>, meta::List<Arguments...>> {
                m_app_name, m_description, tuple_cat(m_options, make_tuple(new_option)), m_arguments
            };
        }

        template<auto member>
        requires(concepts::MemberObjectPointer<decltype(member)> &&
                 concepts::SameAs<Base, meta::MemberPointerClass<decltype(member)>>)
        constexpr auto argument(Optional<StringView> name = {}, Optional<StringView> description = {},
                                bool required = false) {
            auto new_argument = Argument<member> { name, description, required };
            return Parser<Base, meta::List<Options...>, meta::List<Arguments..., decltype(new_argument)>> {
                m_app_name, m_description, m_options, tuple_cat(m_arguments, make_tuple(new_argument))
            };
        }

        constexpr Result<Base> parse(Span<TransparentStringView> args) {
            using namespace di::string_literals;

            if (args.empty()) {
                return Unexpected(BasicError::Invalid);
            }
            args = *args.subspan(1);

            auto seen_arguments = Array<bool, sizeof...(Options)> {};
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
                            return Unexpected(BasicError::Invalid);
                        }

                        // Parse boolean flag.
                        if (option_boolean(*index)) {
                            DI_TRY(option_parse(*index, seen_arguments, &result, {}));
                            continue;
                        }

                        // Parse short option with directly specified value: '-iinput.txt'
                        auto value_view = arg.substr(arg.begin() + char_index + 1);
                        if (!value_view.empty()) {
                            DI_TRY(option_parse(*index, seen_arguments, &result, value_view));
                            break;
                        }

                        // Fail if the is no subsequent arguments left.
                        if (i + 1 >= args.size()) {
                            return Unexpected(BasicError::Invalid);
                        }

                        // Use the next argument as the value.
                        DI_TRY(option_parse(*index, seen_arguments, &result, args[arg_index + 1]));
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
                    return Unexpected(BasicError::Invalid);
                }

                if (option_boolean(*index)) {
                    if (equal) {
                        return Unexpected(BasicError::Invalid);
                    }
                    DI_TRY(option_parse(*index, seen_arguments, &result, {}));
                    send_arg_to_back(arg_index);
                    continue;
                }

                auto value = ""_tsv;
                if (!equal && i + 1 >= args.size()) {
                    return Unexpected(BasicError::Invalid);
                } else if (!equal) {
                    value = args[arg_index + 1];
                    send_arg_to_back(arg_index);
                    send_arg_to_back(arg_index);
                    i++;
                } else {
                    value = arg.substr(equal.end());
                    send_arg_to_back(arg_index);
                }

                DI_TRY(option_parse(*index, seen_arguments, &result, value));
            }

            // Validate all required arguments were processed.
            for (usize i = 0; i < sizeof...(Options); i++) {
                if (!seen_arguments[i] && option_required(i)) {
                    return Unexpected(BasicError::Invalid);
                }
            }

            // All the positional arguments are now at the front of the array.
            auto positional_arguments = *args.subspan(0, args.size() - count_option_processed);
            if (positional_arguments.size() < minimum_required_argument_count()) {
                return Unexpected(BasicError::Invalid);
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

    private:
        constexpr bool option_required(usize index) const {
            return function::index_dispatch<bool, sizeof...(Options)>(index, [&]<usize i>(InPlaceIndex<i>) {
                return util::get<i>(m_options).required();
            });
        }

        constexpr bool option_boolean(usize index) const {
            return function::index_dispatch<bool, sizeof...(Options)>(index, [&]<usize i>(InPlaceIndex<i>) {
                return util::get<i>(m_options).boolean();
            });
        }

        constexpr Result<void> option_parse(usize index, Span<bool> seen_arguments, Base* output,
                                            Optional<TransparentStringView> input) const {
            return function::index_dispatch<Result<void>, sizeof...(Options)>(index,
                                                                              [&]<usize i>(InPlaceIndex<i>) {
                                                                                  return util::get<i>(m_options).parse(
                                                                                      output, input);
                                                                              }) |
                   if_success([&] {
                       seen_arguments[index] = true;
                   });
        }

        constexpr bool argument_variadic(usize index) const {
            return function::index_dispatch<bool, sizeof...(Arguments)>(index, [&]<usize i>(InPlaceIndex<i>) {
                return util::get<i>(m_arguments).variadic();
            });
        }

        constexpr Result<void> argument_parse(usize index, Base* output, Span<TransparentStringView> input) const {
            return function::index_dispatch<Result<void>, sizeof...(Arguments)>(index, [&]<usize i>(InPlaceIndex<i>) {
                return util::get<i>(m_arguments).parse(output, input);
            });
        }

        constexpr usize minimum_required_argument_count() const {
            auto result = usize(0);
            tuple_for_each(
                [&](auto& argument) {
                    result += argument.required_argument_count();
                },
                m_arguments);
            return result;
        }

        constexpr usize argument_count() const { return sizeof...(Arguments); }

        constexpr Optional<usize> lookup_short_name(char short_name) const {
            auto result = Optional<usize> {};
            usize index = 0;
            tuple_for_each(
                [&](auto& flag) {
                    if (flag.short_name() == short_name) {
                        result = index;
                    }
                    index++;
                },
                m_options);
            return result;
        }

        constexpr Optional<usize> lookup_long_name(TransparentStringView long_name) const {
            auto result = Optional<usize> {};
            usize index = 0;
            tuple_for_each(
                [&](auto& flag) {
                    if (flag.long_name() == long_name) {
                        result = index;
                    }
                    index++;
                },
                m_options);
            return result;
        }

        Optional<StringView> m_app_name;
        Optional<StringView> m_description;
        Tuple<Options...> m_options;
        Tuple<Arguments...> m_arguments;
    };
}

template<concepts::Object T>
constexpr auto cli_parser(Optional<StringView> app_name = {}, Optional<StringView> description = {}) {
    return detail::Parser<T, meta::List<>, meta::List<>> { app_name, description, {}, {} };
}
}

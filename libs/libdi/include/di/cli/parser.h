#pragma once

#include <di/cli/option.h>
#include <di/function/prelude.h>

namespace di::cli {
namespace detail {
    template<typename Base, typename Options, typename Arguments>
    class Parser;

    template<concepts::Object Base, typename... Options, typename... Arguments>
    class Parser<Base, meta::List<Options...>, meta::List<Arguments...>> {
    private:
    public:
        constexpr explicit Parser(Optional<StringView> app_name, Optional<StringView> description, Tuple<Options...> options,
                                  Tuple<Arguments...> arguments)
            : m_app_name(app_name), m_description(description), m_options(options), m_arguments(arguments) {}

        template<auto member>
        requires(concepts::MemberObjectPointer<decltype(member)> && concepts::SameAs<Base, meta::MemberPointerClass<decltype(member)>>)
        constexpr auto flag(Optional<char> short_name, Optional<TransparentStringView> long_name = {},
                            Optional<StringView> description = {}) {
            auto new_option = Option<member> { short_name, long_name, description };
            return Parser<Base, meta::List<Options..., decltype(new_option)>, meta::List<Arguments...>> {
                m_app_name, m_description, tuple_cat(m_options, make_tuple(new_option)), m_arguments
            };
        }

        constexpr Result<Base> parse(Span<TransparentStringView> args) {
            using namespace di::string_literals;

            auto seen_arguments = Array<bool, sizeof...(Options)> {};
            seen_arguments.fill(false);

            auto result = Base {};
            for (size_t i = 1; i < args.size(); i++) {
                auto arg = args[i];

                // Handle positional argument.
                if (!arg.starts_with('-')) {
                    return Unexpected(BasicError::Invalid);
                }

                // Handle short argument.
                if (!arg.starts_with("--"_tsv)) {
                    for (size_t char_index = 1; char_index < arg.size(); char_index++) {
                        auto index = lookup_short_name(arg[char_index]);
                        if (!index) {
                            return Unexpected(BasicError::Invalid);
                        }

                        // Parse boolean flag.
                        if (option_boolean(*index)) {
                            DI_TRY(option_parse(*index, &result, {}));
                            continue;
                        }

                        // Parse short option with directly specified value: '-iinput.txt'
                        auto value_view = arg.substr(arg.begin() + char_index + 1);
                        if (!value_view.empty()) {
                            DI_TRY(option_parse(*index, &result, value_view));
                            break;
                        }

                        // Fail if the is no subsequent arguments left.
                        if (i + 1 >= args.size()) {
                            return Unexpected(BasicError::Invalid);
                        }

                        // Use the next argument as the value.
                        DI_TRY(option_parse(*index, &result, args[++i]));
                    }
                    continue;
                }
            }

            // Validate all required arguments were processed.
            for (size_t i = 0; i < sizeof...(Options); i++) {
                if (!seen_arguments[i] && option_required(i)) {
                    return Unexpected(BasicError::Invalid);
                }
            }

            return result;
        }

    private:
        constexpr bool option_required(size_t index) const {
            return function::index_dispatch<bool, sizeof...(Options)>(index, [&]<size_t i>(InPlaceIndex<i>) {
                return util::get<i>(m_options).required();
            });
        }

        constexpr bool option_boolean(size_t index) const {
            return function::index_dispatch<bool, sizeof...(Options)>(index, [&]<size_t i>(InPlaceIndex<i>) {
                return util::get<i>(m_options).boolean();
            });
        }

        constexpr Result<void> option_parse(size_t index, Base* output, Optional<TransparentStringView> input) const {
            return function::index_dispatch<Result<void>, sizeof...(Options)>(index, [&]<size_t i>(InPlaceIndex<i>) {
                return util::get<i>(m_options).parse(output, input);
            });
        }

        constexpr Optional<size_t> lookup_short_name(char short_name) const {
            auto result = Optional<size_t> {};
            size_t index = 0;
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
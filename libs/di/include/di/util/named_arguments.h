#pragma once

#include "di/meta/algorithm.h"
#include "di/meta/constexpr.h"
#include "di/meta/core.h"
#include "di/meta/function.h"
#include "di/meta/language.h"
#include "di/meta/list.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/types/in_place.h"
#include "di/util/get.h"
#include "di/vocab/tuple/tuple.h"

namespace di::util {
/// @brief A helper class to simulate a single named argument.
///
/// @brief Tag The tag type for the named argument.
/// @brief T The type of the named argument.
///
/// This class models a single named argument, where the name is essentially the tag type. Typically, this class is used
/// with CRTP, so the tag type is the derived class.
///
/// @see NamedArguments
template<typename Tag_, typename T>
class NamedArgument {
public:
    using Tag = Tag_;
    using Type = T;

    constexpr static bool is_named_argument = true;

    explicit NamedArgument()
    requires(concepts::DefaultConstructible<T>)
    = default;

    template<typename U>
    requires(!concepts::DecaySameAs<NamedArgument, U> && concepts::ConstructibleFrom<T, U>)
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    constexpr explicit(!concepts::ConvertibleTo<U, T>) NamedArgument(U&& value) : m_value(di::forward<U>(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit NamedArgument(InPlace, Args&&... args) : m_value(di::forward<Args>(args)...) {}

    constexpr auto value() & -> T& { return m_value; }
    constexpr auto value() const& -> T const& { return m_value; }
    constexpr auto value() && -> T&& { return di::move(m_value); }
    constexpr auto value() const&& -> T const&& { return di::move(m_value); }

private:
    T m_value;
};

/// @brief A helper class for simulation named arguments in c++.
///
/// @tparam Args The types of the named arguments.
///
/// This class is used to take a list of named arguments and store them in a form easily accessible by the user. This
/// facility includes the helper functions util::get_named_argument and util::get_named_argument_or which can be used to
/// access the named arguments.
///
/// Named arguments are declared using CRTP with the util::NamedArgument class. The current implementation allows all
/// named arguments to be optional, but this may change in the future.
///
/// The folllowing example shows how to use this class and related methods:
/// @snippet{trimleft} tests/test_util.cpp named_arguments
///
/// @see NamedArgument
/// @see get_named_argument
/// @see get_named_argument_or
template<typename... Args>
requires((meta::RemoveCVRef<Args>::is_named_argument && ...) &&
         meta::Size<meta::Unique<meta::List<meta::Like<Args &&, typename meta::RemoveCVRef<Args>::Tag>...>>> ==
             sizeof...(Args))
class NamedArguments {
public:
    constexpr explicit NamedArguments(Args&&... args) : m_arguments(di::forward<Args>(args)...) {}

    using Type = meta::List<meta::Like<Args&&, typename meta::RemoveCVRef<Args>::Tag>...>;

    constexpr auto arguments() & -> Tuple<Args&&...>& { return m_arguments; }
    constexpr auto arguments() const& -> Tuple<Args&&...> const& { return m_arguments; }
    constexpr auto arguments() && -> Tuple<Args&&...>&& { return di::move(m_arguments); }
    constexpr auto arguments() const&& -> Tuple<Args&&...> const&& { return di::move(m_arguments); }

private:
    Tuple<Args&&...> m_arguments;
};

template<typename... Args>
NamedArguments(Args&&...) -> NamedArguments<Args&&...>;

namespace detail {
    template<typename Tag>
    struct GetNamedArgumentFunction {
        template<concepts::RemoveCVRefInstanceOf<NamedArguments> Args>
        requires(meta::Contains<meta::Type<meta::RemoveCVRef<Args>>, Tag &&>)
        constexpr auto operator()(Args&& args) const -> decltype(auto) {
            constexpr auto index = meta::Lookup<Tag&&, meta::Type<meta::RemoveCVRef<Args>>>;
            return di::get<index>(di::forward<Args>(args).arguments()).value();
        }
    };
}

/// @brief A helper function to access a named argument.
///
/// @tparam Arg The type of the named argument.
///
/// @param args The named arguments to access.
///
/// @return The value of the named argument.
///
/// This function is used to access a named argument from a list of named arguments. Note that it is a compile-time
/// error to call this function with a named argument that is not present in the list of named arguments. Therefore,
/// the concepts::HasNamedArgument concept should be used with an `if constexpr` block before calling this method.
/// Otherwise, use the util::get_named_argument_or function.
///
/// @note This function propogates the value-category of the passed named argument pack, which means that the named
/// argument pack should be passed as an rvalue reference (unless of course, the named argument needs to be read
/// multiple times).
///
/// @see NamedArguments
/// @see get_named_argument_or
template<typename Arg>
constexpr inline auto get_named_argument = detail::GetNamedArgumentFunction<Arg> {};

namespace detail {
    template<typename Tag>
    struct GetNamedArgumentOrFunction {
        template<concepts::RemoveCVRefInstanceOf<NamedArguments> Args, typename Val = meta::Type<Tag>,
                 concepts::ConvertibleTo<Val> U>
        requires(concepts::ConstructibleFrom<Val, meta::Like<Args, Val>>)
        constexpr auto operator()(Args&& args, U&& fallback) const -> Val {
            if constexpr (meta::Contains<meta::Type<meta::RemoveCVRef<Args>>, Tag&&>) {
                return get_named_argument<Tag>(args);
            } else {
                return di::forward<U>(fallback);
            }
        }
    };

    template<typename Tag>
    requires(concepts::InstanceOfT<Tag, InPlaceTemplate>)
    struct GetNamedArgumentOrFunction<Tag> {
        template<concepts::RemoveCVRefInstanceOf<NamedArguments> Args, typename U>
        constexpr auto operator()(Args&& args, U&& fallback) const -> decltype(auto) {
            if constexpr (meta::Contains<meta::Type<meta::RemoveCVRef<Args>>, Tag&&>) {
                return get_named_argument<Tag>(args);
            } else {
                return di::forward<U>(fallback);
            }
        }
    };
}

/// @brief A helper function to access a named argument or a fallback value.
///
/// @tparam Arg The type of the named argument.
///
/// @param args The named arguments to access.
/// @param fallback The fallback value to use if the named argument is not present.
///
/// @return The value of the named argument or the fallback value.
///
/// This function is used to access a named argument from a list of named arguments. If the named argument is not
/// present in the list of named arguments, the fallback value is returned instead. To prevent dangling references, the
/// returned argument is decay copied out.
///
/// @note This function propogates the value-category of the passed named argument pack, which means that, to avoid
/// copies, the named should be passed as an rvalue reference (unless of course, the named argument needs to be read
/// multiple times).
///
/// @note When passed a template argument, the value-category of the returned argument is unchaged, and the return
/// type is inferred based on the presence of the named argument, instead of always returning a common type.
///
/// @see NamedArguments
template<typename Arg>
constexpr inline auto get_named_argument_or = detail::GetNamedArgumentOrFunction<Arg> {};
}

namespace di::meta {
/// @brief A metafunction to the type of a named argument.
///
/// @tparam Args The type of the named argument.
/// @tparam Tag The type of the named argument.
///
/// This metafunction returns the value type of a named argument in a list of named arguments. If the named argument is
/// not present, this will be a compile-time error.
///
/// @see NamedArguments
template<typename Args, typename Tag>
using NamedArgument = decltype(util::get_named_argument<Tag>(declval<Args>()));

/// @brief A metafunction to get the value type of a named argument.
///
/// @tparam Args The type of the named argument.
/// @tparam Tag The type of the named argument.
///
/// This metafunction is equivalent to meta::NamedArgument, but it returns a value instead of a reference.
///
/// @see NamedArguments
template<typename Args, typename Tag>
using NamedArgumentValue = meta::Decay<NamedArgument<Args, Tag>>;

/// @brief A metafunction to get the type of a named argument with fallback.
///
/// @tparam Args The type of the named arguments.
/// @tparam Tag The type of the named argument.
/// @tparam Fallback The fallback type to use if the named argument is not present.
///
/// This metafunction returns the type of a named argument in a list of named arguments. If the named argument is not
/// present in the list of named arguments, the fallback type is returned instead, if Tag is a template argument.
///
/// @see NamedArguments
template<typename Args, typename Tag, typename Fallback>
using NamedArgumentOr = decltype(util::get_named_argument_or<Tag>(declval<Args>(), declval<Fallback>()));

/// @brief A metafunction to get the value type of a named argument with fallback.
///
/// @tparam Args The type of the named arguments.
/// @tparam Tag The type of the named argument.
/// @tparam Fallback The fallback value to use if the named argument is not present.
///
/// This metafunction is equivalent to meta::NamedArgumentOr, but it returns a value instead of a reference.
///
/// @see NamedArguments
template<typename Args, typename Tag, typename Fallback>
using NamedArgumentValueOr = meta::Decay<NamedArgumentOr<Args, Tag, Fallback>>;
}

namespace di::concepts {
/// @brief A concept to check if a named argument is present.
///
/// @tparam Args The type of the named arguments.
/// @tparam Tag The type of the named argument.
///
/// This concept checks if a named argument is present in a list of named arguments.
///
/// @see NamedArguments
template<typename Args, typename Tag>
concept HasNamedArgument = concepts::InstanceOf<Args, util::NamedArguments> && meta::Contains<meta::Type<Args>, Tag&&>;

namespace detail {
    struct ContainsF {
        template<typename List, typename T>
        using Invoke = meta::Constexpr<meta::Contains<List, T>>;
    };
}

/// @brief A concept to check if a list of named arguments is valid.
/// @tparam Allowed The list of allowed named arguments.
/// @tparam Args The list of arguments passed to the function.
///
/// This concept checks if a list of named arguments is valid. A list of named arguments is valid if it contains only
/// named arguments from the list of allowed named arguments. Note that for now, this concept assumes all named
/// arguments are optional.
///
/// @see NamedArguments
template<typename Allowed, typename... Args>
concept ValidNamedArguments =
    (meta::Size<meta::Unique<meta::List<meta::Like<Args, typename Args::Tag>&&...>>> == sizeof...(Args)) &&
    (meta::All<meta::List<meta::Like<Args, typename Args::Tag>&&...>,
               meta::BindFront<detail::ContainsF, meta::Transform<Allowed, meta::Quote<meta::AddRValueReference>>>>);
}

namespace di {
using concepts::HasNamedArgument;
using concepts::ValidNamedArguments;
using util::get_named_argument;
using util::get_named_argument_or;
using util::NamedArgument;
using util::NamedArguments;
}

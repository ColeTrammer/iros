#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/derived_from.h>
#include <di/concepts/instance_of.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/execution/concepts/forwarding_query.h>
#include <di/execution/interface/get_env.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/list/compose.h>
#include <di/meta/list/concepts/type_list.h>
#include <di/meta/list/filter.h>
#include <di/meta/list/list.h>
#include <di/meta/list/quote.h>
#include <di/meta/list/same_as.h>
#include <di/meta/list/type.h>
#include <di/meta/list/unique.h>
#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/vocab/tuple/tuple.h>

namespace di::execution {
namespace make_env_ns {
    template<typename Tag, typename Val>
    struct With {
        using Type = Tag;

        [[no_unique_address]] Val value;
    };

    struct WithFunction {
        template<typename Tag, concepts::CopyConstructible Val>
        constexpr auto operator()(Tag, Val&& value) const {
            return With<Tag, meta::Decay<Val>> { util::forward<Val>(value) };
        }
    };

    template<typename Tag, concepts::TypeList List>
    using LookupTag = meta::Front<meta::Filter<List, meta::Compose<meta::SameAs<Tag>, meta::Quote<meta::Type>>>>;

    template<typename Tag, typename List>
    concept HasTag = requires { typename LookupTag<Tag, List>; };

    template<typename BaseEnv, typename Withs>
    struct EnvT;

    template<typename BaseEnv, typename... Overrides>
    struct EnvT<BaseEnv, meta::List<Overrides...>> {
        struct Type {
        public:
            template<typename... Args>
            constexpr explicit Type(BaseEnv base_env, Overrides... overrides)
                : m_base_env(util::move(base_env)), m_overrides(util::move(overrides)...) {}

        private:
            template<typename Tag>
            requires(HasTag<Tag, meta::List<Overrides...>>)
            constexpr friend auto tag_invoke(Tag, Type const& self) {
                return util::get<LookupTag<Tag, meta::List<Overrides...>>>(self.m_overrides).value;
            }

            template<concepts::ForwardingQuery Tag>
            requires(!HasTag<Tag, meta::List<Overrides...>> && concepts::Invocable<Tag, BaseEnv const&>)
            constexpr friend decltype(auto) tag_invoke(Tag tag, Type const& self) {
                return tag(self.m_base_env);
            }

            [[no_unique_address]] BaseEnv m_base_env;
            [[no_unique_address]] vocab::Tuple<Overrides...> m_overrides;
        };
    };

    template<typename BaseEnv, typename... Withs>
    using Env = meta::Type<EnvT<BaseEnv, meta::List<Withs...>>>;

    struct Function {
        template<typename BaseEnv, concepts::InstanceOf<With>... Overrides>
        constexpr auto operator()(BaseEnv base_env, Overrides... overrides) const {
            static_assert(meta::Size<meta::Unique<meta::List<meta::Type<Overrides>...>>> == sizeof...(Overrides),
                          "di::execution::make_env() must be called with unique overrides.");
            return Env<BaseEnv, Overrides...> { util::move(base_env), util::move(overrides)... };
        }
    };
}

/// @brief Specify an override for an environment query.
///
/// @param tag The tag of the query to override.
/// @param value The value to use for the override.
///
/// @return An override object that can be passed to the `execution::make_env` function
///
/// This function is used as a parameter to `execution::make_env` function to specify an override for a query. The query
/// values are required to be copy constructible, they are decay copied into the environment, and they are copied out
/// when the environment is queried.
///
/// See the execution::with_env() function for an example.
///
/// @see make_env
/// @see with_env
constexpr inline auto with = make_env_ns::WithFunction {};

/// @brief Create an environment with overrides for queries.
///
/// @param base_env The base environment to use for forwarding queries that are not overridden.
/// @param overrides The overrides to use for queries.
///
/// @return An environment with the specified overrides.
///
/// This function creates an environment with the specified overrides. The overrides are specified as a list of objects
/// returned from the `execution::with` function. The overrides are required to be unique.
///
/// This function is also useful when creating an environment with no overrides, as it will only forward queries which
/// have opted-in to be forwarded. This is useful when writng sender algorithms, which don't need to customize the
/// environment but cannot pass the environment through directly to respect the forwarding-ness of the query.
///
/// See the execution::with_env() function for an example.
///
/// @see with
/// @see with_env
constexpr inline auto make_env = make_env_ns::Function {};

/// @brief Represent a override for an environment query.
///
/// @tparam Tag The tag of the query to override.
/// @tparam Val The value to use for the override.
///
/// This type is used as a parameter to `execution::MakeEnv` template to specify an override for a query. This is useful
/// to deduce the return type of the `execution::make_env` function.
///
/// @see make_env
/// @see with
/// @see MakeEnv
template<typename Tag, typename Val>
using With = make_env_ns::With<Tag, Val>;

/// @brief Represent an environment with overrides for queries.
///
/// @tparam BaseEnv The base environment to use for forwarding queries that are not overridden.
/// @tparam Withs The overrides to use for queries.
///
/// This template is used to deduce the return type of the `execution::make_env` function. This is used by sender
/// algorithms which customize the environment to properly query a sender's completion signatures, which vary based on
/// the environment they are invoked in.
///
/// @see make_env
/// @see with
/// @see With
template<typename BaseEnv, typename... Withs>
using MakeEnv = decltype(make_env(util::declval<BaseEnv>(), util::declval<Withs>()...));
}

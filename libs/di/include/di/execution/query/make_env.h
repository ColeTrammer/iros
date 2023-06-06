#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/instance_of.h>
#include <di/execution/concepts/forwarding_query.h>
#include <di/execution/interface/get_env.h>
#include <di/function/tag_invoke.h>
#include <di/meta/list/compose.h>
#include <di/meta/list/concepts/type_list.h>
#include <di/meta/list/filter.h>
#include <di/meta/list/list.h>
#include <di/meta/list/quote.h>
#include <di/meta/list/same_as.h>
#include <di/meta/list/type.h>
#include <di/meta/list/unique.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/vocab/tuple/tuple.h>

namespace di::execution {
namespace make_env_ns {
    template<typename Tag, typename Val>
    struct With {
        using Type = Tag;

        Val value;
    };

    struct WithFunction {
        template<typename Tag, concepts::CopyConstructible Val>
        constexpr auto operator()(Tag, Val&& value) const {
            return With<Tag, Val> { util::forward<Val>(value) };
        }
    };

    template<typename Tag, concepts::TypeList List>
    using LookupTag = meta::Front<meta::Filter<List, meta::Compose<meta::SameAs<Tag>, meta::Quote<meta::Type>>>>;

    template<typename Tag, typename List>
    concept HasTag = requires { typename LookupTag<Tag, List>; };

    template<typename BaseEnv, typename Withs>
    struct EnvT;

    template<typename BaseEnv, typename... Withs>
    struct EnvT<BaseEnv, meta::List<Withs...>> {
        struct Type {
        public:
            template<typename... Args>
            constexpr Type(BaseEnv base_env, Withs... overrides)
                : m_base_env(util::move(base_env)), m_overrides(util::move(overrides)...) {}

        private:
            template<typename Tag>
            requires(HasTag<Tag, meta::List<Withs...>>)
            constexpr friend auto tag_invoke(Tag, Type const& self) {
                return util::get<LookupTag<Tag, meta::List<Withs...>>>(self.m_overrides).value;
            }

            template<concepts::ForwardingQuery Tag>
            requires(!HasTag<Tag, meta::List<Withs...>>)
            constexpr friend decltype(auto) tag_invoke(Tag tag, Type const& self) {
                return tag(self.m_base_env);
            }

            [[no_unique_address]] BaseEnv m_base_env;
            [[no_unique_address]] vocab::Tuple<Withs...> m_overrides;
        };
    };

    template<typename BaseEnv, typename... Withs>
    using Env = meta::Type<EnvT<BaseEnv, meta::List<Withs...>>>;

    struct Function {
        template<typename BaseEnv, concepts::InstanceOf<With>... Withs>
        constexpr auto operator()(BaseEnv base_env, Withs... overrides) const {
            static_assert(meta::Size<meta::Unique<meta::List<meta::Type<Withs>...>>> == sizeof...(Withs),
                          "di::execution::make_env() must be called with unique overrides.");
            return Env<BaseEnv, Withs...> { util::move(base_env), util::move(overrides)... };
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
/// values are required to be copy constructible, and they are copied out when the environment is queried.
///
/// @see make_env
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
/// @see with
constexpr inline auto make_env = make_env_ns::Function {};
}

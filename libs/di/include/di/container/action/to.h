#pragma once

#include "di/container/concepts/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/container/types/prelude.h"
#include "di/container/view/transform.h"
#include "di/function/bind_back.h"
#include "di/meta/operations.h"
#include "di/util/create.h"
#include "di/util/forward.h"

namespace di::container {
namespace detail {
    template<typename Out, typename Con, typename... Args>
    concept DirectConstructTo = concepts::ConvertibleTo<meta::ContainerReference<Con>, meta::ContainerValue<Con>> &&
                                concepts::CreatableFrom<Out, Con, Args...>;

    template<typename Out, typename Con, typename... Args>
    concept TagConstructTo = concepts::ConvertibleTo<meta::ContainerReference<Con>, meta::ContainerValue<Con>> &&
                             concepts::CreatableFrom<Out, FromContainer, Con, Args...>;
}

template<typename Out, concepts::InputContainer Con, typename... Args>
requires(!concepts::View<Out>)
constexpr auto to(Con&& container, Args&&... args) {
    if constexpr (detail::DirectConstructTo<Out, Con, Args...>) {
        return util::create<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
    } else if constexpr (detail::TagConstructTo<Out, Con, Args...>) {
        return util::create<Out>(from_container, util::forward<Con>(container), util::forward<Args>(args)...);
    } else {
        return container::to<Out>(container |
                                  view::transform(
                                      []<typename T>(T&& value) {
                                          return container::to<meta::ContainerValue<Con>>(util::forward<T>(value));
                                      },
                                      util::forward<Args>(args)...));
    }
}

template<template<typename...> typename Template, concepts::InputContainer Con, typename... Args>
requires(concepts::CreateDeducible<Template, Con, Args...> ||
         concepts::CreateDeducible<Template, FromContainer, Con, Args...>)
constexpr auto to(Con&& container, Args&&... args) {
    if constexpr (concepts::CreateDeducible<Template, Con, Args...>) {
        using Out = meta::DeduceCreate<Template, Con, Args...>;
        return container::to<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
    } else {
        using Out = meta::DeduceCreate<Template, Con, Args...>;
        return container::to<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
    }
}

template<typename Out, typename... Args>
requires(!concepts::View<Out>)
constexpr auto to(Args&&... args) {
    return function::bind_back(
        []<concepts::InputContainer Con>(Con&& container, Args&&... args) {
            return container::to<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
        },
        util::forward<Args>(args)...);
}

template<template<typename...> typename Template, typename... Args>
constexpr auto to(Args&&... args) {
    return function::bind_back(
        []<concepts::InputContainer Con>(Con&& container, Args&&... args) {
            return container::to<Template>(util::forward<Con>(container), util::forward<Args>(args)...);
        },
        util::forward<Args>(args)...);
}
}

namespace di {
using container::to;
}

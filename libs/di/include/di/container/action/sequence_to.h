#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/interface/size.h>
#include <di/container/meta/container_size_type.h>
#include <di/container/meta/container_value.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/from_container.h>
#include <di/container/types/prelude.h>
#include <di/container/view/empty_view.h>
#include <di/container/view/transform.h>
#include <di/function/bind_back.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/meta/vocab.h>
#include <di/util/create.h>
#include <di/util/forward.h>
#include <di/vocab/error/meta/common_error.h>
#include <di/vocab/expected/expected_forward_declaration.h>

namespace di::container {
namespace detail {
    template<typename Out, typename Con, typename... Args>
    concept DirectConstructSequenceTo = concepts::CreatableFrom<Out, Con, Args...>;

    template<typename Out, typename Con, typename... Args>
    concept TagConstructSequenceTo = concepts::CreatableFrom<Out, FromContainer, Con, Args...>;

    template<class T>
    concept ReservableContainer = concepts::SizedContainer<T> && requires(T& t, meta::ContainerSizeType<T> n) {
        t.reserve(n);
        { t.capacity() } -> concepts::SameAs<meta::ContainerSizeType<T>>;
        { t.max_size() } -> concepts::SameAs<meta::ContainerSizeType<T>>;
    };

    template<typename Out, typename T>
    constexpr auto do_insert(Out& output, T&& value) {
        if constexpr (requires { output.push_back(di::forward<T>(value)); }) {
            return output.push_back(di::forward<T>(value));
        } else {
            return output.insert(output.end(), di::forward<T>(value));
        }
    }
}

template<typename Out, concepts::InputContainer Con, typename... Args>
requires(!concepts::View<Out> && concepts::Expected<meta::ContainerValue<Con>>)
constexpr auto sequence_to(Con&& container, Args&&... args) {
    if constexpr (detail::DirectConstructSequenceTo<Out, Con, Args...>) {
        return util::create<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
    } else if constexpr (detail::TagConstructSequenceTo<Out, Con, Args...>) {
        return util::create<Out>(from_container, util::forward<Con>(container), util::forward<Args>(args)...);
    } else {
        using InsertResult = decltype(detail::do_insert(di::declval<Out&>(),
                                                        di::declval<meta::ExpectedValue<meta::ContainerValue<Con>>>()));
        using UnwrapResult = meta::ContainerValue<Con>;
        if constexpr (!concepts::Expected<InsertResult>) {
            using Error = meta::ExpectedError<UnwrapResult>;
            return [&] -> Expected<Out, Error> {
                auto output = Out(di::forward<Args>(args)...);

                if constexpr (detail::ReservableContainer<Out> && concepts::SizedContainer<Con>) {
                    output.reserve(size(container));
                }

                for (auto&& value : container) {
                    if (!value) {
                        return Unexpected(di::forward<decltype(value)>(value).error());
                    }
                    detail::do_insert(output, di::forward<decltype(value)>(value).value());
                }
                return output;
            }();
        } else {
            using Error = meta::CommonError<InsertResult, UnwrapResult>;
            return [&] -> Expected<Out, Error> {
                auto output = Out(di::forward<Args>(args)...);

                if constexpr (detail::ReservableContainer<Out> && concepts::SizedContainer<Con>) {
                    DI_TRY(output.reserve(size(container)));
                }

                for (auto&& value : container) {
                    if (!value) {
                        return Unexpected(di::forward<decltype(value)>(value).error());
                    }
                    DI_TRY(detail::do_insert(output, di::forward<decltype(value)>(value).value()));
                }
                return output;
            }();
        }
    }
}

template<template<typename...> typename Template, concepts::InputContainer Con, typename... Args,
         typename UnwrappedContainer = EmptyView<meta::ExpectedValue<meta::ContainerValue<Con>>>>
requires(concepts::CreateDeducible<Template, UnwrappedContainer, Args...> ||
         concepts::CreateDeducible<Template, FromContainer, UnwrappedContainer, Args...>)
constexpr auto sequence_to(Con&& container, Args&&... args) {
    if constexpr (concepts::CreateDeducible<Template, UnwrappedContainer, Args...>) {
        using Out = meta::DeduceCreate<Template, UnwrappedContainer, Args...>;
        return container::sequence_to<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
    } else {
        using Out = meta::DeduceCreate<Template, FromContainer, UnwrappedContainer, Args...>;
        return container::sequence_to<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
    }
}

template<typename Out, typename... Args>
requires(!concepts::View<Out>)
constexpr auto sequence_to(Args&&... args) {
    return function::bind_back(
        []<concepts::InputContainer Con>(Con&& container, Args&&... args) {
            return container::sequence_to<Out>(util::forward<Con>(container), util::forward<Args>(args)...);
        },
        util::forward<Args>(args)...);
}

template<template<typename...> typename Template, typename... Args>
constexpr auto sequence_to(Args&&... args) {
    return function::bind_back(
        []<concepts::InputContainer Con>(Con&& container, Args&&... args) {
            return container::sequence_to<Template>(util::forward<Con>(container), util::forward<Args>(args)...);
        },
        util::forward<Args>(args)...);
}
}

namespace di {
using container::sequence_to;
}

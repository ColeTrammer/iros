#pragma once

#include <di/container/concepts/contiguous_iterator.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/container/meta/container_reference.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/util/forward.h>
#include <di/util/to_address.h>

namespace di::container {
struct DataFunction;

namespace detail {
    template<typename T>
    concept CustomData = concepts::TagInvocableTo<DataFunction, meta::AddPointer<meta::ContainerReference<T>>, T>;

    template<typename T>
    concept MemberData = requires(T&& container) {
        {
            util::forward<T>(container).data()
        } -> concepts::ImplicitlyConvertibleTo<meta::AddPointer<meta::ContainerReference<T>>>;
    };

    template<typename T>
    concept BeginData = concepts::ContiguousIterator<meta::ContainerIterator<T>> && requires(T&& container) {
        {
            util::to_address(container::begin(util::forward<T>(container)))
        } -> concepts::ImplicitlyConvertibleTo<meta::AddPointer<meta::ContainerReference<T>>>;
    };
}

struct DataFunction {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>) &&
             (detail::CustomData<T> || detail::MemberData<T> || detail::BeginData<T>) )
    constexpr meta::AddPointer<meta::ContainerReference<T>> operator()(T&& container) const {
        if constexpr (detail::CustomData<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else if constexpr (detail::MemberData<T>) {
            return util::forward<T>(container).data();
        } else {
            return util::to_address(container::begin(util::forward<T>(container)));
        }
    }
};

constexpr inline auto data = DataFunction {};
}

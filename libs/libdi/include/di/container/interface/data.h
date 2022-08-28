#pragma once

#include <di/concepts/implicitly_convertible_to.h>
#include <di/container/concepts/contiguous_iterator.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/container/meta/container_value.h>
#include <di/meta/add_pointer.h>
#include <di/meta/remove_reference.h>
#include <di/util/forward.h>
#include <di/util/tag_invoke.h>

namespace di::container {
struct DataFunction;

namespace detail {
    template<typename T>
    concept CustomData = concepts::TagInvocableTo<DataFunction, meta::AddPointer<meta::ContainerValue<T>>, T>;

    template<typename T>
    concept MemberData = requires(T&& container) {
                             {
                                 util::forward<T>(container).data()
                                 } -> concepts::ImplicitlyConvertibleTo<meta::AddPointer<meta::ContainerValue<T>>>;
                         };

    template<typename T>
    concept BeginData = concepts::ContiguousIterator<meta::ContainerIterator<T>> &&
                        requires(T&& container) {
                            {
                                begin(util::forward<T>(container))
                                } -> concepts::ImplicitlyConvertibleTo<meta::AddPointer<meta::ContainerValue<T>>>;
                        };
}

struct DataFunction {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<T>) &&
             (detail::CustomData<T> || detail::MemberData<T> || detail::BeginData<T>) )
    constexpr auto operator()(T&& container) const {
        if constexpr (detail::CustomData<T>) {
            return util::tag_invoke(*this, util::forward<T>(container));
        } else if constexpr (detail::MemberData<T>) {
            return util::forward<T>(container).data();
        } else {
            return begin(util::forward<T>(container));
        }
    }
};

constexpr inline auto data = DataFunction {};
}

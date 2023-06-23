#pragma once

#include <di/concepts/common_reference_with.h>
#include <di/meta/add_lvalue_reference.h>
#include <di/meta/common_reference.h>
#include <di/meta/common_type.h>
#include <di/meta/core.h>
#include <di/util/declval.h>

namespace di::concepts {
template<typename T, typename U>
concept CommonWith =
    SameAs<meta::CommonType<T, U>, meta::CommonType<U, T>> &&
    requires {
        static_cast<meta::CommonType<T, U>>(util::declval<T>());
        static_cast<meta::CommonType<T, U>>(util::declval<U>());
    } && CommonReferenceWith<meta::AddLValueReference<T const>, meta::AddLValueReference<U const>> &&
    CommonReferenceWith<meta::AddLValueReference<meta::CommonType<T, U>>,
                        meta::CommonReference<meta::AddLValueReference<T const>, meta::AddLValueReference<U const>>>;
}

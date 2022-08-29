#pragma once

#include <di/meta/false_type.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/true_type.h>
#include <di/util/initializer_list.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct InitializerListHelper : meta::FalseType {};

    template<typename T>
    struct InitializerListHelper<util::InitializerList<T>> : meta::TrueType {};
}

template<typename T>
concept InitializerList = detail::InitializerListHelper<meta::RemoveCVRef<T>>::value;
}

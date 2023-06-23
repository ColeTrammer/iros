#pragma once

#include <di/any/concepts/method.h>
#include <di/any/meta/method_signature.h>
#include <di/any/types/prelude.h>
#include <di/meta/list/prelude.h>
#include <di/meta/remove_cvref.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    constexpr static bool is_interface = false;

    template<typename... Methods>
    requires((concepts::Method<meta::Type<Methods>> && ...) &&
             (meta::ExactlyOnce<meta::Transform<meta::AsList<meta::MethodSignature<meta::Type<Methods>>>,
                                                meta::Quote<meta::RemoveCVRef>>,
                                This> &&
              ...))
    constexpr static bool is_interface<meta::List<Methods...>> = true;
}

template<typename T>
concept Interface = detail::is_interface<T>;
}

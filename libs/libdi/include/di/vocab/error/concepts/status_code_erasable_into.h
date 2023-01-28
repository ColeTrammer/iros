#pragma once

#include <di/concepts/language_void.h>
#include <di/meta/remove_cvref.h>
#include <di/vocab/error/meta/status_code_domain_value.h>
#include <di/vocab/error/status_code_storage.h>

namespace di::concepts {
namespace detail {
    template<typename From, typename To>
    concept StatusCodeErasableIntoHelper = (!LanguageVoid<From> && !LanguageVoid<To> &&
                                            TriviallyRelocatable<meta::StatusCodeDomainValue<From>> &&
                                            TriviallyRelocatable<meta::StatusCodeDomainValue<To>> &&
                                            sizeof(vocab::detail::StatusCodeStorage<From>) <=
                                                sizeof(vocab::detail::StatusCodeStorage<To>));
}

template<typename From, typename To>
concept StatusCodeErasableInto = detail::StatusCodeErasableIntoHelper<meta::RemoveCVRef<From>, meta::RemoveCVRef<To>>;
}
#pragma once

#include <di/any/concepts/method.h>
#include <di/any/meta/method_signature.h>
#include <di/meta/language_function_return.h>
#include <di/meta/list/prelude.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<concepts::Method Method>
using MethodErasedSignature =
    meta::AsLanguageFunction<meta::LanguageFunctionReturn<meta::MethodSignature<Method>>,
                             meta::ReplaceIf<meta::AsList<meta::MethodSignature<Method>>,
                                             meta::Compose<meta::SameAs<This>, meta::Quote<meta::RemoveCVRef>>, void*>>;
}

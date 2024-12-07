#pragma once

#include "di/any/concepts/method.h"
#include "di/any/meta/method_signature.h"
#include "di/meta/algorithm.h"
#include "di/meta/core.h"
#include "di/meta/language.h"

namespace di::meta {
template<concepts::Method Method>
using MethodErasedSignature =
    meta::AsLanguageFunction<meta::LanguageFunctionReturn<meta::MethodSignature<Method>>,
                             meta::ReplaceIf<meta::AsList<meta::MethodSignature<Method>>,
                                             meta::Compose<meta::SameAs<This>, meta::Quote<meta::RemoveCVRef>>, void*>>;
}

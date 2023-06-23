#pragma once

#include <di/concepts/language_array.h>
#include <di/concepts/language_function.h>
#include <di/meta/add_pointer.h>
#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_extent.h>
#include <di/meta/remove_reference.h>

namespace di::meta {
template<typename T>
using Decay = Conditional<
    concepts::LanguageArray<RemoveReference<T>>, RemoveExtent<RemoveReference<T>>*,
    Conditional<concepts::LanguageFunction<RemoveReference<T>>, AddPointer<RemoveReference<T>>, RemoveCVRef<T>>>;
}

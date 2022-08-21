#pragma once

#include <di/util/concepts/language_array.h>
#include <di/util/concepts/language_function.h>
#include <di/util/meta/add_pointer.h>
#include <di/util/meta/conditional.h>
#include <di/util/meta/remove_cvref.h>
#include <di/util/meta/remove_extent.h>
#include <di/util/meta/remove_reference.h>

namespace di::util::meta {
template<typename T>
using Decay = Conditional<concepts::LanguageArray<RemoveReference<T>>, RemoveExtent<RemoveReference<T>>*,
                          Conditional<concepts::LanguageFunction<RemoveReference<T>>, AddPointer<RemoveReference<T>>, RemoveCVRef<T>>>;
}

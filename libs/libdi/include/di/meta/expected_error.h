#pragma once

#include <di/concepts/expected.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<concepts::Expected T>
using ExpectedError = meta::RemoveCVRef<T>::Error;
}

#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>
#include <di/vocab/error/status_code_forward_declaration.h>

namespace di::concepts {
template<typename T>
concept StatusCode = InstanceOf<meta::RemoveCVRef<T>, vocab::StatusCode>;
}

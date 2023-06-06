#pragma once

#include <di/concepts/instance_of.h>
#include <di/meta/remove_cvref.h>
#include <di/vocab/expected/expected_forward_declaration.h>

namespace di::concepts {
template<typename T>
concept Expected = InstanceOf<meta::RemoveCVRef<T>, vocab::Expected>;
}

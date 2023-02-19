#pragma once

#include <di/prelude.h>
#include <iris/core/error.h>

namespace iris {
Expected<di::Span<di::Byte const>> lookup_in_initrd(di::PathView path);
}
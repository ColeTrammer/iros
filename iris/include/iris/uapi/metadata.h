#pragma once

#include <di/types/prelude.h>

namespace iris {
enum class MetadataType {
    Regular = 1,
    Directory = 2,
};

struct Metadata {
    MetadataType type;
    u64 size;
};
}

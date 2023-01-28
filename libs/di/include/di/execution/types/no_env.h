#pragma once

#include <di/concepts/same_as.h>

namespace di::types {
struct NoEnv {
    friend void tag_invoke(auto, concepts::SameAs<NoEnv> auto, auto&&...) = delete;
};
}
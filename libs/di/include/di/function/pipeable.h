#pragma once

#include "di/meta/core.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"

namespace di::function::pipeline {
struct EnablePipeline {};

template<typename T>
concept Pipeable =
    concepts::DerivedFrom<meta::RemoveCVRef<T>, EnablePipeline> && concepts::ConstructibleFrom<T, meta::Decay<T>>;
}

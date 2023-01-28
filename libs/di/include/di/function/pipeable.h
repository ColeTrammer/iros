#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/derived_from.h>
#include <di/meta/decay.h>
#include <di/meta/remove_cvref.h>

namespace di::function::pipeline {
struct EnablePipeline {};

template<typename T>
concept Pipeable =
    concepts::DerivedFrom<meta::RemoveCVRef<T>, EnablePipeline> && concepts::ConstructibleFrom<T, meta::Decay<T>>;
}

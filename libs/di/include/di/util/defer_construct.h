#pragma once

#include "di/function/invoke.h"
#include "di/meta/util.h"

namespace di::util {
template<concepts::Invocable Fun>
class DeferConstruct {
public:
    template<typename T>
    requires(!concepts::DecaysTo<T, DeferConstruct> && concepts::ConstructibleFrom<Fun, T>)
    constexpr explicit DeferConstruct(T&& value) : m_function(util::forward<T>(value)) {}

    constexpr operator meta::InvokeResult<Fun>() && { return function::invoke(util::forward<Fun>(m_function)); }

private:
    Fun m_function;
};

template<typename T>
DeferConstruct(T) -> DeferConstruct<T>;
}

namespace di {
using util::DeferConstruct;
}

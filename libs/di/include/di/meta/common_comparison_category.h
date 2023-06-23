#pragma once

#include <di/meta/core.h>
#include <di/types/partial_ordering.h>
#include <di/types/strong_ordering.h>
#include <di/types/weak_ordering.h>

namespace di::meta {
namespace detail {
    template<typename... Types>
    struct CommonComparisonCategoryHelper : TypeConstant<void> {};

    template<>
    struct CommonComparisonCategoryHelper<> : TypeConstant<types::strong_ordering> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<types::partial_ordering, Types...> : TypeConstant<types::partial_ordering> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<types::weak_ordering, Types...>
        : TypeConstant<Conditional<
              concepts::SameAs<types::partial_ordering, typename CommonComparisonCategoryHelper<Types...>::Type>,
              types::partial_ordering, types::weak_ordering>> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<types::strong_ordering, Types...>
        : CommonComparisonCategoryHelper<Types...> {};
}

template<typename... Types>
using CommonComparisonCategory = detail::CommonComparisonCategoryHelper<Types...>::Type;
}

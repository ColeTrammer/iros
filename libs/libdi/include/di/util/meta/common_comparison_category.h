#pragma once

#include <di/util/concepts/same_as.h>
#include <di/util/meta/conditional.h>
#include <di/util/meta/type_constant.h>
#include <di/util/types/partial_ordering.h>
#include <di/util/types/strong_ordering.h>
#include <di/util/types/weak_ordering.h>

namespace di::util::meta {
namespace detail {
    template<typename... Types>
    struct CommonComparisonCategoryHelper : TypeConstant<void> {};

    template<>
    struct CommonComparisonCategoryHelper<> : TypeConstant<types::strong_ordering> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<types::partial_ordering, Types...> : TypeConstant<types::partial_ordering> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<types::weak_ordering, Types...>
        : TypeConstant<Conditional<concepts::SameAs<types::partial_ordering, typename CommonComparisonCategoryHelper<Types...>::Type>,
                                   types::partial_ordering, types::weak_ordering>> {};

    template<typename... Types>
    struct CommonComparisonCategoryHelper<types::strong_ordering, Types...> : CommonComparisonCategoryHelper<Types...> {};
}

template<typename... Types>
using CommonComparisonCategory = detail::CommonComparisonCategoryHelper<Types...>::Type;
}

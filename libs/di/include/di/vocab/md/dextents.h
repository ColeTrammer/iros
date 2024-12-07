#pragma once

#include "di/meta/algorithm.h"
#include "di/meta/constexpr.h"
#include "di/types/prelude.h"
#include "di/vocab/md/extents_forward_declaration.h"

namespace di::vocab {
namespace detail {
    template<typename SizeType, typename Ind>
    struct DextentsHelper;

    template<typename SizeType, usize... extents>
    struct DextentsHelper<SizeType, meta::List<Constexpr<extents>...>>
        : meta::TypeConstant<Extents<SizeType, extents...>> {};
}

template<typename SizeType, usize rank>
using Dextents = detail::DextentsHelper<SizeType, meta::Repeat<Constexpr<dynamic_extent>, rank>>::Type;
}

namespace di {
using vocab::Dextents;
}

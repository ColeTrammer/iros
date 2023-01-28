#pragma once

#include <di/meta/list/prelude.h>
#include <di/meta/size_constant.h>
#include <di/vocab/md/extents_forward_declaration.h>

namespace di::vocab {
namespace detail {
    template<typename SizeType, typename Ind>
    struct DextentsHelper;

    template<typename SizeType, size_t... extents>
    struct DextentsHelper<SizeType, meta::List<meta::SizeConstant<extents>...>>
        : meta::TypeConstant<Extents<SizeType, extents...>> {};
}

template<typename SizeType, size_t rank>
using Dextents = detail::DextentsHelper<SizeType, meta::Repeat<meta::SizeConstant<dynamic_extent>, rank>>::Type;
}
#pragma once

#include <di/container/concepts/contiguous_container.h>
#include <di/container/concepts/iterator.h>
#include <di/container/interface/data.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/container/interface/possibly_const_container.h>
#include <di/container/meta/const_iterator.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/remove_reference.h>
#include <di/util/as_const_pointer.h>
#include <di/util/forward.h>

namespace di::container {
struct CDataFunction {
    template<concepts::ContiguousContainer T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>))
    constexpr auto operator()(T&& container) const {
        return util::as_const_pointer(container::data(detail::possibly_const_container(container)));
    }
};

constexpr inline auto cdata = CDataFunction {};
}

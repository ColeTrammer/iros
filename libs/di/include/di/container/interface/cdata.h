#pragma once

#include <di/container/concepts/contiguous_container.h>
#include <di/container/concepts/iterator.h>
#include <di/container/interface/data.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/container/interface/possibly_const_container.h>
#include <di/container/meta/const_iterator.h>
#include <di/function/pipeable.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/util/as_const_pointer.h>
#include <di/util/forward.h>

namespace di::container {
struct CDataFunction : function::pipeline::EnablePipeline {
    template<concepts::ContiguousContainer T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>))
    constexpr auto operator()(T&& container) const {
        return util::as_const_pointer(container::data(detail::possibly_const_container(container)));
    }
};

constexpr inline auto cdata = CDataFunction {};
}

namespace di {
using container::cdata;
}

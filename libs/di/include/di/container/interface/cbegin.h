#pragma once

#include <di/container/concepts/iterator.h>
#include <di/container/interface/begin.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/container/interface/possibly_const_container.h>
#include <di/container/meta/const_iterator.h>
#include <di/function/pipeable.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/util.h>
#include <di/util/forward.h>

namespace di::container {
struct CBeginFunction : function::pipeline::EnablePipeline {
    template<concepts::InputContainer T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>))
    constexpr auto operator()(T&& container) const {
        return meta::ConstIterator<decltype(container::begin(detail::possibly_const_container(container)))>(
            container::begin(detail::possibly_const_container(container)));
    }
};

constexpr inline auto cbegin = CBeginFunction {};
}

namespace di {
using container::cbegin;
}

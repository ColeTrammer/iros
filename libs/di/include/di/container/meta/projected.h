#pragma once

#include <di/container/concepts/indirectly_readable.h>
#include <di/container/concepts/indirectly_regular_unary_invocable.h>
#include <di/container/iterator/iterator_ssize_type.h>
#include <di/container/iterator/iterator_value.h>
#include <di/container/meta/indirect_result.h>
#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>
#include <di/util/declval.h>

namespace di::meta {
template<concepts::IndirectlyReadable It, concepts::IndirectlyRegularUnaryInvocable<It> Proj>
struct Projected {
    using Reference = meta::IndirectResult<Proj&, It>;
    using Value = meta::RemoveCVRef<Reference>;
    using SSizeType = meta::IteratorSSizeType<It>;

    Reference operator*() const;

    constexpr friend InPlaceType<Value> tag_invoke(types::Tag<container::iterator_value>, InPlaceType<Projected>) {
        return in_place_type<Value>;
    }
    constexpr friend SSizeType tag_invoke(types::Tag<container::iterator_ssize_type>, InPlaceType<Projected>) {
        return util::declval<SSizeType>();
    }
};
}

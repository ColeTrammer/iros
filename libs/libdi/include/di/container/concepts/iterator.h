#pragma once

#include <di/concepts/movable.h>
#include <di/concepts/same_as.h>
#include <di/container/meta/iterator_category.h>
#include <di/container/meta/iterator_size_type.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/container/meta/iterator_value.h>
#include <di/meta/remove_cvref.h>
#include <di/util/as_const.h>

namespace di::concepts::detail {
template<typename Iter>
concept Iterator = Movable<Iter> && requires(meta::RemoveCVRef<Iter> it) {
                                        typename meta::IteratorCategory<Iter>;
                                        typename meta::IteratorValue<Iter>;
                                        typename meta::IteratorSSizeType<Iter>;
                                        typename meta::IteratorSizeType<Iter>;
                                        { ++it } -> SameAs<meta::RemoveCVRef<Iter>&>;
                                        it++;
                                        { *util::as_const(it) } -> SameAs<meta::IteratorValue<Iter>>;
                                    };
}

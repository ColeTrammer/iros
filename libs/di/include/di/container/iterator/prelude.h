#pragma once

#include <di/container/iterator/advance.h>
#include <di/container/iterator/default_sentinel.h>
#include <di/container/iterator/distance.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/move_iterator.h>
#include <di/container/iterator/next.h>
#include <di/container/iterator/prev.h>
#include <di/container/iterator/reverse_iterator.h>
#include <di/container/iterator/sentinel_base.h>

namespace di {
using container::advance;
using container::distance;
using container::next;
using container::prev;

using container::default_sentinel;
using container::DefaultSentinel;
}

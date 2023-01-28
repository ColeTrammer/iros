#pragma once

#include <di/container/view/adjacent.h>
#include <di/container/view/adjacent_transform.h>
#include <di/container/view/all.h>
#include <di/container/view/as_const.h>
#include <di/container/view/as_rvalue.h>
#include <di/container/view/cartesian_product.h>
#include <di/container/view/chunk.h>
#include <di/container/view/chunk_by.h>
#include <di/container/view/common.h>
#include <di/container/view/concat.h>
#include <di/container/view/counted.h>
#include <di/container/view/cycle.h>
#include <di/container/view/drop.h>
#include <di/container/view/drop_while.h>
#include <di/container/view/elements.h>
#include <di/container/view/empty.h>
#include <di/container/view/enumerate.h>
#include <di/container/view/filter.h>
#include <di/container/view/join.h>
#include <di/container/view/join_with.h>
#include <di/container/view/keys.h>
#include <di/container/view/pairwise.h>
#include <di/container/view/pairwise_transform.h>
#include <di/container/view/range.h>
#include <di/container/view/repeat.h>
#include <di/container/view/reverse.h>
#include <di/container/view/single.h>
#include <di/container/view/slide.h>
#include <di/container/view/split.h>
#include <di/container/view/stride.h>
#include <di/container/view/take.h>
#include <di/container/view/take_while.h>
#include <di/container/view/transform.h>
#include <di/container/view/values.h>
#include <di/container/view/view.h>
#include <di/container/view/zip.h>
#include <di/container/view/zip_transform.h>

namespace di {
namespace container::view {}

namespace view = container::view;

using container::View;

using view::adjacent;
using view::adjacent_transform;
using view::as_rvalue;
using view::cartesian_product;
using view::chunk;
using view::chunk_by;
using view::common;
using view::concat;
using view::cycle;
using view::drop;
using view::drop_while;
using view::elements;
using view::enumerate;
using view::filter;
using view::iota;
using view::join;
using view::join_with;
using view::keys;
using view::pairwise;
using view::pairwise_transform;
using view::range;
using view::repeat;
using view::reverse;
using view::single;
using view::slide;
using view::split;
using view::stride;
using view::take;
using view::take_while;
using view::transform;
using view::values;
using view::zip;
using view::zip_transform;
}

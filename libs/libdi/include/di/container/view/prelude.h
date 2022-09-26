#pragma once

#include <di/container/view/all.h>
#include <di/container/view/as_rvalue.h>
#include <di/container/view/counted.h>
#include <di/container/view/drop.h>
#include <di/container/view/drop_while.h>
#include <di/container/view/empty.h>
#include <di/container/view/filter.h>
#include <di/container/view/range.h>
#include <di/container/view/repeat.h>
#include <di/container/view/reverse.h>
#include <di/container/view/single.h>
#include <di/container/view/take.h>
#include <di/container/view/take_while.h>
#include <di/container/view/transform.h>
#include <di/container/view/view.h>
#include <di/container/view/zip.h>

namespace di {
namespace container::view {}

namespace view = container::view;

using container::View;
using view::as_rvalue;
using view::drop;
using view::drop_while;
using view::filter;
using view::iota;
using view::range;
using view::repeat;
using view::reverse;
using view::single;
using view::take;
using view::take_while;
using view::transform;
using view::zip;
}

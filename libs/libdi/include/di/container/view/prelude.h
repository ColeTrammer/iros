#pragma once

#include <di/container/view/all.h>
#include <di/container/view/as_rvalue.h>
#include <di/container/view/empty.h>
#include <di/container/view/range.h>
#include <di/container/view/repeat.h>
#include <di/container/view/reverse.h>
#include <di/container/view/single.h>
#include <di/container/view/transform.h>
#include <di/container/view/view.h>

namespace di {
namespace container::view {}

namespace view = container::view;

using container::View;
using view::as_rvalue;
using view::range;
using view::repeat;
using view::reverse;
using view::single;
using view::transform;
}

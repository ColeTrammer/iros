#pragma once

#include <di/container/view/all.h>
#include <di/container/view/empty.h>
#include <di/container/view/range.h>
#include <di/container/view/repeat.h>
#include <di/container/view/single.h>
#include <di/container/view/view.h>

namespace di {
namespace container::view {}

namespace view = container::view;

using container::View;
using view::range;
using view::repeat;
using view::single;
}

#pragma once

#include "di/container/view/elements_view.h"

namespace di::container {
template<typename Con>
using ValuesView = ElementsView<Con, 1>;
}

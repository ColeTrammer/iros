#pragma once

#include "di/container/view/elements_view.h"

namespace di::container {
template<typename Con>
using KeysView = ElementsView<Con, 0>;
}

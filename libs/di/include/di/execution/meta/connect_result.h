#pragma once

#include <di/execution/interface/connect.h>

namespace di::meta {
template<typename Send, typename Rec>
using ConnectResult = decltype(execution::connect(util::declval<Send>(), util::declval<Rec>()));
}

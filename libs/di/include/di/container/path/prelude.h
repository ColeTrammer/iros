#pragma once

#include <di/container/path/path.h>
#include <di/container/path/path_view.h>

namespace di {
using container::PathView;
using container::Utf8PathView;

using container::Path;
using container::Utf8Path;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_PATH_LITERALS)
using namespace di::literals::path_view_literals;
#endif

#pragma once

#include <ext/error.h>
#include <liim/forward.h>

namespace Ext {
Result<String, SystemError> realpath(StringView path);
}

#include <ext/system.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <unistd.h>

namespace Ext {
Result<String, SystemError> realpath(StringView view) {
    auto path = String(view);
    char* result = ::realpath(path.string(), nullptr);
    if (!result) {
        return Err(make_system_error_from_errno());
    }
    auto ret = String(result);
    free(result);
    return ret;
}
}

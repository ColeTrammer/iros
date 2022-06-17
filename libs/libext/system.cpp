#include <ext/system.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <unistd.h>

namespace Ext {
Result<String, SystemError> realpath(StringView view) {
    auto path = String(view);
    char* result = ::realpath(path.string(), nullptr);
    if (!result) {
        return Err(SystemError::from_errno(make_vector({ move(path) })));
    }
    auto ret = String(result);
    free(result);
    return ret;
}
}

#pragma once

#include <ext/forward.h>
#include <liim/forward.h>

namespace Ext {
class Error {
public:
    virtual ~Error() {}
    virtual StringView type() const;
    virtual String to_message() const;
};
}

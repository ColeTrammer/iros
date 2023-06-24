#pragma once

namespace di::types {
struct This {
private:
    This() = default;
};
}

namespace di {
using types::This;
}

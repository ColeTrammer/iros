#pragma once

namespace di::util::concepts {
template<bool... values>
concept Conjunction = (values && ...);
}

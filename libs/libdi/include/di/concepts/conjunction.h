#pragma once

namespace di::concepts {
template<bool... values>
concept Conjunction = (values && ...);
}

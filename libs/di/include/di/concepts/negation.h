#pragma once

namespace di::concepts {
template<bool value>
concept Negation = (!value);
}

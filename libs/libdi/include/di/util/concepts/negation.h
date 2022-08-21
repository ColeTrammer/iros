#pragma once

namespace di::util::concepts {
template<bool value>
concept Negation = !value;
}

#pragma once

namespace di::util::concepts {
template<bool... values>
concept Disjunction = (values || ...);
}

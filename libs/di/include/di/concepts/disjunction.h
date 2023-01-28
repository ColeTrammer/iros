#pragma once

namespace di::concepts {
template<bool... values>
concept Disjunction = (values || ...);
}

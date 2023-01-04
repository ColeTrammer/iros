#pragma once

namespace di::vocab {
template<typename... Types>
requires(sizeof...(Types) > 0)
class Variant;
}
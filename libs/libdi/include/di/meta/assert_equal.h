#pragma once

namespace di::meta {
template<auto a, auto b>
requires(a == b)
struct AssertEqual {};
}

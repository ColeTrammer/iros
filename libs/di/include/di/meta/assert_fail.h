#pragma once

namespace di::meta {
template<auto...>
requires(false)
struct AssertFail {};
}

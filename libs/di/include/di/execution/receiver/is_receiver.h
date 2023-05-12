#pragma once

namespace di::execution {
template<typename R>
constexpr inline bool enable_receiver = requires { typename R::is_receiver; };
}

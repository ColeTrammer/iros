#pragma once

namespace di::concepts {
template<typename T>
concept Trait = requires { typename T::Type; };
}

#pragma once

namespace di::concepts {
namespace details {
    template<typename T, template<auto...> typename Template>
    constexpr inline bool instance_of_v_helper = false;

    template<auto... values, template<auto...> typename Template>
    constexpr inline bool instance_of_v_helper<Template<values...>, Template> = true;
}

template<typename T, template<auto...> typename Template>
concept InstanceOfV = details::instance_of_helper<T, Template>;
}
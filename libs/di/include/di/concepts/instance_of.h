#pragma once

namespace di::concepts {
namespace details {
    template<typename T, template<typename...> typename Template>
    constexpr inline bool instance_of_helper = false;

    template<typename... Types, template<typename...> typename Template>
    constexpr inline bool instance_of_helper<Template<Types...>, Template> = true;
}

template<typename T, template<typename...> typename Template>
concept InstanceOf = details::instance_of_helper<T, Template>;
}

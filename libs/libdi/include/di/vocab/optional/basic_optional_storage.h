#pragma once

#include <di/util/concepts/constructible_from.h>
#include <di/util/concepts/same_as.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/forward.h>
#include <di/util/forward_like.h>
#include <di/util/meta/decay.h>
#include <di/vocab/optional/get_value.h>
#include <di/vocab/optional/is_nullopt.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/set_nullopt.h>
#include <di/vocab/optional/set_value.h>

namespace di::vocab::optional {
template<typename T>
class BasicOptionalStorage {
public:
    constexpr explicit BasicOptionalStorage(NullOpt) {}

    constexpr BasicOptionalStorage(BasicOptionalStorage const&) = default;
    constexpr BasicOptionalStorage(BasicOptionalStorage&&) = default;

    constexpr BasicOptionalStorage& operator=(BasicOptionalStorage const&) = default;
    constexpr BasicOptionalStorage& operator=(BasicOptionalStorage&&) = default;

private:
    constexpr friend bool tag_invoke(util::Tag<is_nullopt>, BasicOptionalStorage const& self) { return !self.m_has_value; }

    template<typename Self>
    requires(util::concepts::SameAs<util::meta::Decay<Self>, BasicOptionalStorage>)
    constexpr friend decltype(auto) tag_invoke(util::Tag<get_value>, Self&& self) {
        return util::forward_like<Self>(self.m_value);
    }

    constexpr friend void tag_invoke(util::Tag<set_nullopt>, BasicOptionalStorage& self) {
        if (self.m_has_value) {
            util::destroy_at(&self.m_value);
        }
        self.m_has_value = false;
    }

    template<typename... Args>
    requires(util::concepts::ConstructibleFrom<T, Args...>)
    constexpr friend void tag_invoke(util::Tag<set_value>, BasicOptionalStorage& self, Args&&... args) {
        util::construct_at(&self.m_value, util::forward<Args>(args)...);
        self.m_has_value = true;
    }

    bool m_has_value { false };
    union {
        T m_value;
    };
};
}

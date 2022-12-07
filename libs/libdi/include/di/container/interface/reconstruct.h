#pragma once

#include <di/concepts/class.h>
#include <di/concepts/language_array.h>
#include <di/container/concepts/iterator.h>
#include <di/container/concepts/view.h>
#include <di/container/interface/enable_borrowed_container.h>
#include <di/container/view/view.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/remove_reference.h>
#include <di/util/forward.h>

namespace di::container {
struct ReconstructFunction;

namespace detail {
    template<typename... Args>
    concept CustomReconstruct = concepts::TagInvocable<ReconstructFunction, Args...>;

    template<typename It, typename Sent>
    concept ViewReconstruct = concepts::ConstructibleFrom<View<It, Sent>, It, Sent>;
};

struct ReconstructFunction {
    template<typename It, typename Sent>
    requires((concepts::Class<It> || concepts::Class<Sent> || concepts::Enum<It> || concepts::Enum<Sent>) &&
             (detail::CustomReconstruct<It, Sent> || detail::ViewReconstruct<It, Sent>) )
    constexpr concepts::View auto operator()(It&& iterator, Sent&& sentinel) const {
        if constexpr (detail::CustomReconstruct<It, Sent>) {
            return function::tag_invoke(*this, util::forward<It>(iterator), util::forward<Sent>(sentinel));
        } else {
            return View<It, Sent>(util::forward<It>(iterator), util::forward<Sent>(sentinel));
        }
    }

    template<concepts::Container Con, typename It, typename Sent>
    constexpr concepts::View auto operator()(InPlaceType<Con>, It&& iterator, Sent&& sentinel) const
    requires(detail::CustomReconstruct<InPlaceType<Con>, It, Sent> ||
             requires {
                 { (*this)(util::forward<It>(iterator), util::forward<Sent>(sentinel)) } -> concepts::View;
             })
    {
        if constexpr (detail::CustomReconstruct<InPlaceType<Con>, It, Sent>) {
            return function::tag_invoke(*this, in_place_type<Con>, util::forward<It>(iterator), util::forward<Sent>(sentinel));
        } else {
            return (*this)(util::forward<It>(iterator), util::forward<Sent>(sentinel));
        }
    }

    template<concepts::Container Con, concepts::Container T, typename It, typename Sent>
    constexpr concepts::View auto operator()(InPlaceType<Con>, T&& container, It&& iterator, Sent&& sentinel) const
    requires(detail::CustomReconstruct<InPlaceType<Con>, T, It, Sent> ||
             requires {
                 { (*this)(in_place_type<Con>, util::forward<It>(iterator), util::forward<Sent>(sentinel)) } -> concepts::View;
             })
    {
        if constexpr (detail::CustomReconstruct<InPlaceType<Con>, It, Sent>) {
            return function::tag_invoke(*this, in_place_type<Con>, util::forward<T>(container), util::forward<It>(iterator),
                                        util::forward<Sent>(sentinel));
        } else {
            return (*this)(in_place_type<Con>, util::forward<It>(iterator), util::forward<Sent>(sentinel));
        }
    }
};

constexpr inline auto reconstruct = ReconstructFunction {};
}

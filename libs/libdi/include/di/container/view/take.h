#pragma once

#include <di/container/algorithm/min.h>
#include <di/container/interface/reconstruct.h>
#include <di/container/interface/ssize.h>
#include <di/container/view/take_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct TakeFunction {
        template<concepts::ViewableContainer Con, concepts::ConvertibleTo<meta::ContainerSSizeType<Con>> Diff>
        constexpr concepts::View auto operator()(Con&& container, Diff&& difference) const {
            if constexpr (concepts::TagInvocable<TakeFunction, Con, Diff>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<Diff>(difference));
            } else if constexpr (concepts::SizedContainer<Con> && concepts::RandomAccessContainer<Con> &&
                                 concepts::BorrowedContainer<Con> &&
                                 concepts::ContainerIteratorReconstructibleContainer<Con, meta::RemoveCVRef<Con>,
                                                                                     meta::ContainerIterator<Con>,
                                                                                     meta::ContainerIterator<Con>>) {
                return container::reconstruct(
                    in_place_type<meta::RemoveCVRef<Con>>, util::forward<Con>(container), container::begin(container),
                    container::begin(container) +
                        container::min(container::ssize(container),
                                       static_cast<meta::ContainerSSizeType<Con>>(difference)));
            } else {
                return TakeView { util::forward<Con>(container),
                                  static_cast<meta::ContainerSSizeType<Con>>(difference) };
            }
        }
    };
}

constexpr inline auto take = function::curry_back(detail::TakeFunction {}, meta::size_constant<2>);
}
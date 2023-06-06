#pragma once

#include <di/container/view/chunk_view.h>
#include <di/function/curry_back.h>

namespace di::container::view {
namespace detail {
    struct ChunkFunction;

    template<typename Con, typename SSizeType>
    concept CustomChunk = concepts::TagInvocable<ChunkFunction, Con, SSizeType>;

    template<typename Con, typename SSizeType>
    concept ViewChunk = requires(Con&& container, SSizeType&& predicate) {
        ChunkView { util::forward<Con>(container), util::forward<SSizeType>(predicate) };
    };

    struct ChunkFunction {
        template<concepts::ViewableContainer Con, typename SSizeType>
        requires(CustomChunk<Con, SSizeType> || ViewChunk<Con, SSizeType>)
        constexpr concepts::View auto operator()(Con&& container, SSizeType&& predicate) const {
            if constexpr (CustomChunk<Con, SSizeType>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<SSizeType>(predicate));
            } else {
                return ChunkView { util::forward<Con>(container), util::forward<SSizeType>(predicate) };
            }
        }
    };
}

constexpr inline auto chunk = function::curry_back(detail::ChunkFunction {}, meta::c_<2zu>);
}

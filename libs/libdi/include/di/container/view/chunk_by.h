#pragma once

#include <di/container/view/chunk_by_view.h>
#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct ChunkByFunction;

    template<typename Con, typename Pred>
    concept CustomChunkBy = concepts::TagInvocable<ChunkByFunction, Con, Pred>;

    template<typename Con, typename Pred>
    concept ViewChunkBy = requires(Con&& container, Pred&& predicate) {
                              ChunkByView { util::forward<Con>(container), util::forward<Pred>(predicate) };
                          };

    struct ChunkByFunction {
        template<concepts::ViewableContainer Con, typename Pred>
        requires(CustomChunkBy<Con, Pred> || ViewChunkBy<Con, Pred>)
        constexpr concepts::View auto operator()(Con&& container, Pred&& predicate) const {
            if constexpr (CustomChunkBy<Con, Pred>) {
                return function::tag_invoke(*this, util::forward<Con>(container), util::forward<Pred>(predicate));
            } else {
                return ChunkByView { util::forward<Con>(container), util::forward<Pred>(predicate) };
            }
        }
    };
}

constexpr inline auto chunk_by = function::curry_back(detail::ChunkByFunction {}, meta::size_constant<2>);
}
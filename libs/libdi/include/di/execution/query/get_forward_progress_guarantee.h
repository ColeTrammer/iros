#include <di/execution/concepts/scheduler.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
enum class ForwardProgressGuarantee {
    Concurrent,
    Parallel,
    WeaklyParallel,
};

namespace detail {
    struct GetForwardProgressGuaranteeFunction {
        template<concepts::Scheduler Sched>
        constexpr ForwardProgressGuarantee operator()(Sched const& scheduler) const {
            if constexpr (concepts::TagInvocable<GetForwardProgressGuaranteeFunction, Sched const&>) {
                static_assert(
                    concepts::TagInvocable<GetForwardProgressGuaranteeFunction, Sched const&>,
                    "Customizations of get_forward_progress_guarantee() must return di::ForwardProgressGuarantee.");
                return function::tag_invoke(*this, scheduler);
            } else {
                return ForwardProgressGuarantee::WeaklyParallel;
            }
        }
    };
}

constexpr inline auto get_forward_progress_guarantee = detail::GetForwardProgressGuaranteeFunction {};
}
#pragma once

#include <di/execution/algorithm/into_variant.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/sequence/transform_each.h>
#include <di/function/pipeable.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace into_variant_each_ns {
    struct Function : function::pipeline::EnablePipeline {
        template<concepts::Sender Seq>
        auto operator()(Seq&& sequence) const {
            if constexpr (concepts::TagInvocable<Function, Seq>) {
                return tag_invoke(*this, util::forward<Seq>(sequence));
            } else {
                return transform_each(util::forward<Seq>(sequence), into_variant);
            }
        }
    };
}

/// @brief Transform the values of a sequence into a variant.
///
/// @param sequence The sequence to transform.
///
/// @returns A sequence sender that can only send a single value.
///
/// This function is like execution::into_variant(), but for sequences.
///
/// @see into_variant
/// @see transform_each
constexpr inline auto into_variant_each = into_variant_each_ns::Function {};
}

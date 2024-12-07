#pragma once

#include "di/execution/algorithm/just.h"
#include "di/execution/algorithm/let.h"

namespace di::execution {
namespace stopped_as_error_ns {
    struct Function {
        template<concepts::Sender Send, concepts::MovableValue Error>
        auto operator()(Send&& sender, Error&& error) const -> concepts::Sender auto {
            return execution::let_stopped(util::forward<Send>(sender), [error = util::forward<Error>(error)] {
                return execution::just_error(util::move(error));
            });
        }
    };
}

constexpr inline auto stopped_as_error = function::curry_back(stopped_as_error_ns::Function {}, meta::c_<2ZU>);
}

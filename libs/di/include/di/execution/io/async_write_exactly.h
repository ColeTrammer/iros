#pragma once

#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/let.h>
#include <di/execution/algorithm/let_value_with.h>
#include <di/execution/algorithm/repeat_effect.h>
#include <di/execution/algorithm/then.h>
#include <di/execution/io/async_write_some.h>
#include <di/execution/receiver/set_value.h>

namespace di::execution {
namespace async_write_exactly_ns {
    struct Function {
        template<typename File>
        concepts::SenderOf<SetValue()> auto operator()(File&& handle, Span<Byte const> buffer,
                                                       Optional<u64> offset = {}) const
        requires(requires { async_write_some(util::forward<File>(handle), buffer, offset); })
        {
            if constexpr (concepts::TagInvocable<Function, File, Span<Byte const>, Optional<u64>>) {
                return function::tag_invoke(*this, util::forward<File>(handle), buffer, offset);
            } else {
                return execution::just(util::forward<File>(handle), buffer, offset, false) |
                       execution::let_value([](auto& handle, Span<Byte const>& buffer, Optional<u64>& offset,
                                               bool& should_stop) {
                           return execution::async_write_some(handle, buffer, offset) |
                                  execution::then([&buffer, &offset, &should_stop](size_t nwritten) -> Result<void> {
                                      if (nwritten == 0) {
                                          return Unexpected(BasicError::ResultOutOfRange);
                                      }
                                      buffer = *buffer.subspan(nwritten);
                                      if (offset) {
                                          *offset += nwritten;
                                      }
                                      should_stop = buffer.empty();
                                      return {};
                                  }) |
                                  execution::repeat_effect_until([&should_stop] {
                                      return should_stop;
                                  });
                       });
            }
        }
    };
}

constexpr inline auto async_write_exactly = async_write_exactly_ns::Function {};
}

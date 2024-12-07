#pragma once

#include "di/container/allocator/allocate_one.h"
#include "di/container/allocator/allocator.h"
#include "di/container/allocator/deallocate_one.h"
#include "di/container/allocator/fallible_allocator.h"
#include "di/container/allocator/infallible_allocator.h"
#include "di/execution/interface/connect.h"
#include "di/execution/interface/get_env.h"
#include "di/execution/interface/start.h"
#include "di/execution/meta/connect_result.h"
#include "di/execution/query/get_allocator.h"
#include "di/execution/query/get_completion_scheduler.h"
#include "di/execution/query/make_env.h"
#include "di/execution/receiver/set_stopped.h"
#include "di/execution/receiver/set_value.h"
#include "di/execution/sequence/sequence_sender.h"
#include "di/execution/types/empty_env.h"
#include "di/function/container/prelude.h"
#include "di/function/tag_invoke.h"
#include "di/meta/core.h"
#include "di/platform/compiler.h"
#include "di/util/addressof.h"
#include "di/util/construct_at.h"
#include "di/util/destroy_at.h"
#include "di/util/immovable.h"
#include "di/vocab/expected/as_fallible.h"
#include "di/vocab/expected/try_infallible.h"

namespace di::execution {
namespace start_detached_ns {
    template<typename Alloc>
    struct DataT {
        struct Type {
            Function<void()> did_complete;
            [[no_unique_address]] Alloc allocator;
        };
    };

    template<typename Alloc>
    using Data = meta::Type<DataT<Alloc>>;

    template<typename Alloc>
    struct ReceiverT {
        struct Type {
            using is_receiver = void;

            Data<Alloc>* data;

            friend void tag_invoke(Tag<set_value>, Type&& self) { self.data->did_complete(); }
            friend void tag_invoke(Tag<set_stopped>, Type&& self) { self.data->did_complete(); }

            friend auto tag_invoke(Tag<get_env>, Type const& self) {
                return make_env(empty_env, with(get_allocator, self.data->allocator));
            }
        };
    };

    template<typename Alloc>
    using Receiver = meta::Type<ReceiverT<Alloc>>;

    template<typename Send, typename Alloc>
    struct StorageT {
        struct Type : util::Immovable {
            using Rec = Receiver<Alloc>;
            using Op = meta::ConnectResult<Send, Rec>;

            explicit Type(Alloc&& allocator, Send&& sender)
                : data(
                      [this] {
                          // Destroy the operation state when the sender completes. Since we store the allocator
                          // ourselves, we must first move it out of the operation state before destroying ourselves.
                          auto allocator = util::move(data.allocator);
                          auto* pointer = this;
                          util::destroy_at(pointer);
                          container::deallocate_one<Type>(allocator, pointer);
                      },
                      util::forward<Alloc>(allocator))
                , op(connect(util::forward<Send>(sender), Rec(util::addressof(data)))) {}

            [[no_unique_address]] Data<Alloc> data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op op;
        };
    };

    template<typename Send, typename Alloc>
    using Storage = meta::Type<StorageT<Send, Alloc>>;

    struct Function {
        template<concepts::NextSender Send, concepts::Allocator Alloc = platform::DefaultAllocator>
        auto operator()(Send&& sender, Alloc&& allocator = {}) const {
            if constexpr (requires {
                              tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                         util::forward<Send>(sender), util::forward<Alloc>(allocator));
                          }) {
                return tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                  util::forward<Send>(sender), util::forward<Alloc>(allocator));
            } else if constexpr (requires {
                                     tag_invoke(*this, util::forward<Send>(sender), util::forward<Alloc>(allocator));
                                 }) {
                return tag_invoke(*this, util::forward<Send>(sender), util::forward<Alloc>(allocator));
            } else {
                using Store = Storage<Send, Alloc>;

                return vocab::as_fallible(container::allocate_one<Store>(allocator)) % [&](Store* storage) {
                    util::construct_at(storage, util::forward<Alloc>(allocator), util::forward<Send>(sender));
                    start(storage->op);
                } | vocab::try_infallible;
            }
        }
    };
}

/// @brief Start a sender without waiting for it to complete.
///
/// @param sender The sender to start.
/// @param allocator The allocator to use for the operation state (optional).
///
/// @return Possibly an error indicating the operaiton state could not be allocated.
///
/// This function is used to start a sender in a fire-and-forget manner. The operation state is heap allocated
/// using the provided allocator. If no allocator is provided, the default allocator is used. If allocation is
/// failible, this function will return an error.
///
/// This function is like execution::spawn() but does not require a scope to be provided. This means that the
/// sender may remain pending for an arbitrary amount of time, and the operation state will not be destroyed
/// until the sender completes. As such, this function should be used with care, since it can easily lead to
/// unpredictable resource usage.
///
/// @note The sender must not send any values or complete with an error (since the result is ignored). The only
/// completion signatures allowed are di::SetValue() and di::SetStopped().
///
/// @see spawn
constexpr inline auto start_detached = start_detached_ns::Function {};
}

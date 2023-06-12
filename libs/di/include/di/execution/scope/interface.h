#pragma once

#include <di/concepts/boolean_testable.h>
#include <di/concepts/maybe_fallible.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/function/tag_invoke.h>
#include <di/meta/unwrap_expected.h>
#include <di/meta/unwrap_reference.h>
#include <di/util/reference_wrapper.h>
#include <di/util/unwrap_reference.h>

namespace di::execution {
namespace request_stop_ns {
    template<typename Scope>
    concept MemberInvocable = requires(meta::UnwrapReference<Scope>& scope) { scope.request_stop(); };

    struct Function {
        template<typename Scope>
        requires(MemberInvocable<Scope> || concepts::TagInvocable<Function, meta::UnwrapReference<Scope>&>)
        bool operator()(Scope& scope) const {
            if constexpr (MemberInvocable<Scope>) {
                static_assert(concepts::BooleanTestable<decltype(unwrap_reference(scope).request_stop())>,
                              "request_stop() member function must return a boolean.");
                return unwrap_reference(scope).request_stop();
            } else {
                static_assert(concepts::BooleanTestable<meta::TagInvokeResult<Function, meta::UnwrapReference<Scope>&>>,
                              "request_stop() customizations must return a boolean.");
                return tag_invoke(*this, unwrap_reference(scope));
            }
        }
    };
}

/// @brief Request that a scope stop.
///
/// @param scope The scope to request stop on.
///
/// @return true if the scope was stopped, false if it was already.
///
/// This is used to request that a scope stop. In most cases, this is done by forwarding the request to the
/// scope's underlyign stop source. Since this is a CPO, this provides a uniform interface to this functionality.
constexpr inline auto request_stop = request_stop_ns::Function {};

namespace nest_ns {
    struct Function {
        template<typename Scope, concepts::Sender Send>
        requires(concepts::TagInvocable<Function, meta::UnwrapReference<Scope>&, Send>)
        auto operator()(Scope& scope, Send&& sender) const {
            static_assert(concepts::Sender<meta::TagInvokeResult<Function, meta::UnwrapReference<Scope>&, Send&&>>,
                          "nest() customizations must return a Sender.");
            return tag_invoke(*this, unwrap_reference(scope), util::forward<Send>(sender));
        }
    };
}

/// @brief Nest a sender inside a scope.
///
/// @param scope The scope to nest the sender inside.
/// @param sender The sender to nest.
///
/// @return A sender that will run the sender inside the scope.
///
/// This is the most primitive way to nest a sender inside a scope. Nesting a sender inside of a scope means that
/// the sender is required to complete before the scope is destroyed. This is useful in cases where we need to use a
/// scope to manage the lifetime of a sender, as is the case any time the number of senders to be run is not known
/// at compile time.
///
/// This function does not allocate any memory, and is therefore the most efficient way to nest a sender inside a
/// scope, but is also the most inconvenient. In most cases, it is more desirable to use execution::spawn() or
/// execution::spawn_future() instead. However, these functions can be implemented in terms of this function.
///
/// @see spawn
/// @see spawn_future
constexpr inline auto nest = nest_ns::Function {};

namespace spawn_ns {
    struct Function {
        template<typename Scope, concepts::NextSender<meta::EnvOf<Scope>> Send>
        requires(concepts::TagInvocable<Function, meta::UnwrapReference<Scope>&, Send>)
        auto operator()(Scope& scope, Send&& sender) const {
            static_assert(
                concepts::MaybeFallible<meta::TagInvokeResult<Function, meta::UnwrapReference<Scope>&, Send&&>, void>,
                "spawn() customizations must return a maybe fallible void.");
            return tag_invoke(*this, unwrap_reference(scope), util::forward<Send>(sender));
        }
    };
}

/// @brief Spawn a sender inside a scope.
///
/// @param scope The scope to spawn the sender inside.
/// @param sender The sender to spawn.
///
/// @return void if the sender was spawned successfully, otherwise an error.
///
/// This function is used to spawn a sender inside a scope. Spawning a sender inside a scope means that the sender is
/// eagerly started, and the scope will not be destroyed until the sender completes. This is useful in cases where we
/// need to run a dynamic amount of work, and make sure not to leak resources.
///
/// This function allocates the operation to be run on the heap, and is therefore less efficient than using
/// execution::nest(). However, it is more convenient, since it eagerly starts the sender, and still manages the memory
/// properly. However, for this reason, if the underlying allocator is fallible, this function can return an error.
///
/// In cases where the result of the sender is needed, execution::spawn_future() should be used instead.
///
/// @note The sender must not send any values or complete with an error (since the result is ignored). The only
/// completion signatures allowed are di::SetValue() and di::SetStopped().
///
/// @see nest
/// @see spawn_future
constexpr inline auto spawn = spawn_ns::Function {};

namespace spawn_future_ns {
    struct Function {
        template<typename Scope, concepts::SenderIn<meta::EnvOf<Scope>> Send>
        requires(concepts::TagInvocable<Function, meta::UnwrapReference<Scope>&, Send>)
        auto operator()(Scope& scope, Send&& sender) const {
            static_assert(
                concepts::Sender<
                    meta::UnwrapExpected<meta::TagInvokeResult<Function, meta::UnwrapReference<Scope>&, Send&&>>>,
                "nest() customizations must return a maybe fallible Sender.");
            return tag_invoke(*this, unwrap_reference(scope), util::forward<Send>(sender));
        }
    };
}

/// @brief Spawn a sender inside a scope, and return a future to the result.
///
/// @param scope The scope to spawn the sender inside.
/// @param sender The sender to spawn.
///
/// @return A sender to the result of the sender.
///
/// This function is used to spawn a sender inside a scope, and return a future to the result. Spawning a sender means
/// it is eagerly started, and the scope will not be destroyed until the sender completes. This differs from
/// execution::nest(), which does not eagerly start the sender.
///
/// This function is useful in cases where the result of the sender is needed. However, it is less efficient than both
/// execution::nest() and execution::spawn(), since it allocates the operation to be run on the heap, and must resolve
/// the race condition between the eagerly started sender completing, and the returned future being started.
///
/// @see nest
/// @see spawn
constexpr inline auto spawn_future = spawn_future_ns::Function {};
}

#pragma once

#include <di/execution/concepts/async_creatable_from.h>
#include <di/execution/concepts/async_destroyable.h>
#include <di/execution/concepts/awaitable_sender.h>
#include <di/execution/concepts/completion_signature.h>
#include <di/execution/concepts/execution_context.h>
#include <di/execution/concepts/forwarding_query.h>
#include <di/execution/concepts/is_awaitable.h>
#include <di/execution/concepts/is_awaiter.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/scheduler.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/concepts/sender_of.h>
#include <di/execution/concepts/sender_to.h>
#include <di/execution/concepts/single_sender.h>
#include <di/execution/concepts/valid_completion_signatures.h>

namespace di {
using concepts::Receiver;
using concepts::ReceiverOf;
using concepts::Scheduler;
using concepts::Sender;
using concepts::SenderIn;
using concepts::SenderOf;
using concepts::SenderTo;
}

#pragma once

#include <di/execution/meta/single_sender_value_type.h>

namespace di::concepts {
template<typename Send, typename Env = types::NoEnv>
concept SingleSender = Sender<Send, Env> && requires { typename meta::SingleSenderValueType<Send, Env>; };
}

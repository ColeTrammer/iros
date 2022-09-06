#pragma once

#include <di/function/bind_back.h>
#include <di/function/bind_front.h>
#include <di/function/compose.h>
#include <di/function/curry.h>
#include <di/function/curry_back.h>
#include <di/function/id.h>
#include <di/function/invoke.h>
#include <di/function/monad.h>
#include <di/function/piped.h>
#include <di/function/pipeline.h>
#include <di/function/tag_invoke.h>

namespace di {
using function::bind_back;
using function::bind_front;
using function::compose;
using function::curry;
using function::curry_back;
using function::id;
using function::piped;
using function::unit;

using concepts::TagInvocable;
using concepts::TagInvocableTo;
using function::tag_invoke;
using meta::TagInvokeResult;
using types::Tag;

using concepts::Invocable;
using concepts::InvocableTo;
using function::invoke;
using function::invoke_r;
using meta::InvokeResult;
}

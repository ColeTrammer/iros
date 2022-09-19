#pragma once

#include <di/function/bind_back.h>
#include <di/function/bind_front.h>
#include <di/function/compare.h>
#include <di/function/compose.h>
#include <di/function/curry.h>
#include <di/function/curry_back.h>
#include <di/function/equal.h>
#include <di/function/equal_or_greater.h>
#include <di/function/equal_or_less.h>
#include <di/function/greater.h>
#include <di/function/id.h>
#include <di/function/invoke.h>
#include <di/function/less.h>
#include <di/function/monad/prelude.h>
#include <di/function/not_equal.h>
#include <di/function/piped.h>
#include <di/function/pipeline.h>
#include <di/function/tag_invoke.h>
#include <di/function/ycombinator.h>

namespace di {
using function::bind_back;
using function::bind_front;
using function::compose;
using function::curry;
using function::curry_back;
using function::id;
using function::piped;
using function::ycombinator;

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

using function::Compare;
using function::Equal;
using function::EqualOrGreater;
using function::EqualOrLess;
using function::Greater;
using function::Less;
using function::NotEqual;

using function::compare;
using function::equal;
using function::equal_or_greater;
using function::equal_or_less;
using function::greater;
using function::less;
using function::not_equal;
}

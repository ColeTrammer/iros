#pragma once

#include <di/function/bind_back.h>
#include <di/function/bind_front.h>
#include <di/function/bit_and.h>
#include <di/function/compare.h>
#include <di/function/compare_backwards.h>
#include <di/function/compose.h>
#include <di/function/curry.h>
#include <di/function/curry_back.h>
#include <di/function/equal.h>
#include <di/function/equal_or_greater.h>
#include <di/function/equal_or_less.h>
#include <di/function/flip.h>
#include <di/function/generator.h>
#include <di/function/greater.h>
#include <di/function/identity.h>
#include <di/function/into_void.h>
#include <di/function/invoke.h>
#include <di/function/less.h>
#include <di/function/minus.h>
#include <di/function/monad/prelude.h>
#include <di/function/multiplies.h>
#include <di/function/not_equal.h>
#include <di/function/not_fn.h>
#include <di/function/overload.h>
#include <di/function/piped.h>
#include <di/function/pipeline.h>
#include <di/function/plus.h>
#include <di/function/tag_invoke.h>
#include <di/function/uncurry.h>
#include <di/function/value.h>
#include <di/function/ycombinator.h>

namespace di {
using function::bind_back;
using function::bind_front;
using function::compose;
using function::curry;
using function::curry_back;
using function::flip;
using function::not_fn;
using function::overload;
using function::piped;
using function::uncurry;
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

using function::BitAnd;
using function::Compare;
using function::Equal;
using function::EqualOrGreater;
using function::EqualOrLess;
using function::Greater;
using function::Identity;
using function::Less;
using function::Minus;
using function::Multiplies;
using function::NotEqual;
using function::Plus;

using function::bit_and;
using function::compare;
using function::compare_backwards;
using function::equal;
using function::equal_or_greater;
using function::equal_or_less;
using function::greater;
using function::identity;
using function::less;
using function::minus;
using function::multiplies;
using function::not_equal;
using function::plus;

using function::into_void;

using function::Generator;
}

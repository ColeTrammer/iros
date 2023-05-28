# Execution

## Purpose

The execution module is designed to allow asynchronous computation in an efficent and composable way. This library uses
the design proposed in [P2300](https://github.com/brycelelbach/wg21_p2300_execution/tree/main), except no execeptions
are used.

## Conceptual Overview

The core component of this library is senders, which represent an asynchronous computation. Senders are composable using
provided algorithms, and can their execution can be controlled using schedulers.

Senders can complete in one of three ways:

1. With values
2. With an error
3. Stopped (cancellation)

This result is communicated to a receiver, which is like a callback that is invoked when the sender completes.

A third concept is used to actually start an asynchronous computation, called an operation state. This is the result of
connecting a sender to a receiver, and is used to start the computation.
Before the operation state is started, no work is done.

## Type Erased Sender

The `di::AnySender` class template is a type erased sender, meaning it can hold any sender that satisfies the
requirements.

```cpp
// This is a type erased sender that can hold any sender that can complete with exactly an i32.
using MySender = di::AnySender<
    di::CompletionSignatures<di::SetValue(i32)>
>;

auto x = MySender(di::just(5));
auto y = MySender(next_keyboard_scan_code());
```

```cpp
// This is a type erased sender that can hold any sender that can complete with an i32, void, or an error.
using MySender = di::AnySender<
    di::CompletionSignatures<di::SetValue(i32), di::SetValue(), di::SetError(di::Error)>;

auto x = MySender(di::just(5));
auto y = MySender(di::just());
auto z = MySender(di::just_error(di::Error(di::BasicError::InvalidArgument)));
```

The key point is that AnySender can hold any sender that can complete with a strict subset of the allowed signatures.

### How does this work?

A key aspect of this model is that senders can connect to any receiver which accepts its completion signature. This
normally results in a lot of templates, which cannot be represented in a type erased context. To solve this, there is
also a type erased receiver, which can hold any receiver that accepts the completion signature of the sender.

Furthermore, the result of connecting a sender to a receiver is an operation state, which will also be different for
each sender-receiver pair. There is therefore also a type-erased operation state.

Lastly, senders, receivers, and operation states are all queryable, and the resulting environment object must also be
type-erased.

### Problems with this Approach

The main problem with this approach is that it will require heap-allocations for sufficently large senders, receivers,
and operation states. What's more, allocations can fail, and the library does not allow throwing exceptions. This
creates a problem, because the `di::connect` CPO is required to return a valid operation state, and this will require a
heap allocation.

#### Case 1: Creating the Type-Erased Receiver Fails

There is really no choice but to simply refuse to compile code if the type-erased receiver conversion is fallible. The
only potential alternative would be to immediately invoke the receiver with an error, but this could cause asyncrhonous
computations to start before the operation state is started, which breaks the entire model.

#### Case 2: Creating the Type-Erased Sender Fails

The good news is that this is not really a problem. The `di::connect` CPO will be called with the type-erased sender
already existing, so there is no way for this to fail. However, creating the sender in the first place could fail. This
implies that functions returning a type-erased sender would have to return a `di::Result<di::AnySender<...>>`, which is
not ideal, especially since this model already encompasses errors. It would be a lot better to simply return a
`di::AnySender<...>`, and have the error be communicated through the operation state. This is possible, but it requires
making `di::Expected<Sender, E>` a valid sender, with completion signatures equivalent to `Sender` with the addition of
`di::SetError(E)`. This is not ideal, because it would also mean that this type would need some variant `connect`
function would return a variant operation state, which either holds the operation state of the sender, or the error.

#### Case 3: Creating the Type-Erased Operation State Fails

This is the most difficult case to deal with. The problem is that the `di::connect` CPO is required to return a valid
operation state, and this will require a heap allocation. The only way to "fix" this is to make a dummy operation state,
which when started, immediately invokes the receiver with an error. This is not ideal, because it means that the normal
type-erasure mechanism cannot be used, since custom logic needs to be put into the `di::connect` function. Practially,
the library can be modified to make `di::Expected<OperationState, E>` a valid operation state, with the type-erasure
code will have to just deal with this. An additional problem which arises is that objects which model `OperationState`
are not movable, so even just creating an `Expected` object will prove difficult. The probable way to solve this is too
simply always heap allocate the `OperationState`, which is unfortunate.

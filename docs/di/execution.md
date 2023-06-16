# Execution

## Purpose

The execution module is designed to allow asynchronous computation in an efficent and composable way. This library uses
the design proposed in [P2300](https://github.com/brycelelbach/wg21_p2300_execution/tree/main), except that no
execeptions are used.

## Conceptual Overview

The core component of this library is senders, which represent an asynchronous computation. Senders are composable using
provided algorithms, and can their execution can be controlled using schedulers.

Senders can complete in one of three ways:

1. With values
2. With an error
3. Stopped (cancellation)

This result is communicated to a receiver, which is like a callback that is invoked when the sender completes.

The third concept is a scheduler, which is used to control the execution of a sender. This is used to explicitly model
where a sender is executed. For instance, a sender can be executed on a thread pool, or even on a GPU. The scheduler is
accessible using the `di::execution::schedule()` customization point object (CPO), which produces a sender which
completes on the scheduler. Any work chained to this sender will also be executed on the scheduler.

### Life Time Model

The sender/receiver model obeys structured concurrency, which enables asynchronous operations to be composed safely
without any form of garbage collection (or using shared pointers). In fact, a scheduler for Linux's io_uring is capable
of scheduling tasks, while performing async io, all without any memory allocations (or even system calls).

They key behind this model is that senders are not started until they are connected to a receiver, and the resulting
operation state is explicitly started. The operation state itself is an immovable type which cannot be destroyed until
the sender completes (either with a value, error, or stopped). This means that the operation state can be allocated on
stack, in cases where the caller waits for the sender to complete. Additionally, the sender itself uses the operation
state to store everything needed to complete the computation, which means the sender is free to be destroyed once it is
connected to a receiver. The receiver itself will be stored in the operation state, and will be destroyed when the
computation completes.

The operation state being immovable is like the `Pin<T>` type from rust, and is especially useful because it means that
the operation states can be stored in a linked-list of stack allocated variables. This enables the sender/receiver model
to schedule work without allocating any memory.

## Async Sequences

An extension to the sender model is the async sequence. This is a sender which itself completes when the entire sequence
finishes. Each individual element of the sequence is communicated to the receiver using the `di::execution::set_next()`
CPO, which is passed an lvalue receiver and a sender which sends the next element of the sequence. This mechanism is
modelled after a draft c++ [standard proposal](https://github.com/kirkshoop/sequence-next).

A key aspect of this model is that the outer sender can only complete once all of the inner senders have completed. This
ensures structured concurrency, and allows the outer sender to perform cleanup operations once all of the inner senders
have completed. Cancellation can prevent new inner senders from being started, but any pending inner senders still must
complete before the outer sender can complete.

### Who calls set_next()?

The `di::execution::set_next()` CPO is called by the sequence sender whenever it determines that there is another
element in the sequence. For instance, if a sequence sender represents a server listening to sockets, it can call
set_next() whenever it makes a request to accept a new socket (i.e. with io_uring). Then this sender completes when the
connection is actually established. Some sequences do not need to compute the next element asynchronously, and can
provide `di::execution::just(values...)` as the next sender.

Additionally, multiple next senders can be in-flight simultaneously. This is useful for sequences which can compute on
multiple threads. Since this is controlled by the sequence itself, sequences can also guarantee that only one next
sender is in-flight at a time, which might allow optimizations.

The return value of `di::execution::set_next()` is an new sender, which must connected and started. Normally, this is
done by the producer itself directly after calling `set_next()`, which connects the return value to its own receiver.
This acts as a hook to fire off more work when that sender finishes. The associated operation state is also normally
started immediately. In the case where there is only a single sender in-flight at a time, the operation state can be
stored directly, but if there were a dynamic number of senders in-flight, the operation states may need to be heap
allocated.

### set_next() Allows the Receiver to Communicate Back to the Sequence

The `di::execution::set_next()` CPO not only informs the receiver of the next element in the sequence, but also returns
a sender which the sequence must handle. This sender either completes with a void value, which indicates that the item
was accepted, or it completes with `di::execution::SetStopped()`, which indicates that the sequence should stop sending
new elements.

Since this outcome is itself modelled by a sender, it is an asynchronous computation. This allows the receiver to
communicate back-pressure to the sequence. For instance, if the sequence is a server which is listening to sockets, the
consumer can simply not complete the sender returned by `set_next()` until it is ready to accept a new socket. Sequences
which enforce a maximum number of in-flight next senders can use this mechanism to ensure that the sequence does not
start too many next senders.

### Async Sequence Life Time Model

Like in the case of regular senders, the operation state of an async sequence is immovable, and must not be destroyed
before the sequence completes. However, each individual next sender has its own lifetime and associated operation state.
The key point is that all of the next senders must complete before the sequence can complete, and the sequence must
ensure that the operation states of the next senders are not destroyed before they complete.

In practice, this means that before the final completion can be reported, the sequence must wait for all of the next
senders to complete, and then it can destroy the operation states of the next senders. Because next senders can complete
in parallel (and on different threads), there is a requirement that the completion of all of the next senders strongly
happens before the the sequence can complete. This can be accomplished by having an atomic counter which decrements when
a next sender completes, and it is sufficent to use acquire/release memory ordering for this counter.

The upside of this model is that the sequence can be allocated on the stack, and there is never any chance of dangling
pointers, since the operation state's have guaranteed lifetimes. The downside is that the sequence must wait for all of
the next senders to complete before it can complete, which means that cancelling the sequence actually requires some
work. This can also be considered desireable since it ensures that cleanup operations are performed when needed, and not
in a distant point in the future when a shared pointer's ref count reaches zero.

### Lockstep Sequences

Although the model allows for multiple next senders to be in-flight at the same time, in many cases, the underlying
sequence ensures that only one next sender is in-flight at a time. This property can be used to optimize algorithms by
removing the need for synchronization. For instance, an algorithm like `execution::fold()`, which can be used to compute
the sum of a sequence, needs to ensure the fold function is not called concurrently. If the sequence is a lockstep
sequence, no synchronization is needed. Sequences can opt-in to this optimization by providing overriding the query
`execution::is_always_lockstep_sequence` to return true in their associated environment.

The library provides the `execution::into_lockstep_sequence()` CPO, which can be used to convert a parallel sequence
into a lockstep sequence. This is used internally for any algorithm like `execution::fold()` which requires a lockstep
sequences.

### How do completion signatures work with sequences?

Since all sequences complete with a call to `di::execution::set_value()`, this completion signature is implied. Instead,
the reported value completion signatures are the completion signatures of the next sender. The error and stopped
signatures can be propogated as the overall result of the sequence, and are also valid completions for the next sender
(but the return value from set_next() must transform these errors into a value or stopped completion).

### Comparison with libunifex Models

[libunifex](https://github.com/facebookexperimental/libunifex) provides two models for sending multiple values: many
sender and async stream.

The many sender model allows senders to complete multiple times, which is useful for parallelism. However, the
individual items are not really modelled by an asyncronous process, and there is no way to perform cleanup.

The async stream model is similar to the async sequence model, but it does not allow multiple next senders to be in
flight at the same time. This means that the sequence must be able to compute the next element in series, which makes
parallelism difficult (but it can be done using queueing). For cleanup, there is an explicit CPO which algorithms need
to call, which is not ideal.

Additionally, the async stream is poll-based instead of push-based. This means that someone needs to call `next()` on
stream to get a new element, which is constrasts with regaular senders which push elements to the receiver.

### Drawbacks of the Async Sequence Model

The main tradeoff of the async sequence model is that since multiple next senders can be in-flight at the same time,
there needs to be synchronization in some algorithms. This is also true for the many sender model, in algorithms like
`di::execution::when_all()`. But this may be more common in the async sequence model. On the other hand, as long as the
synchronization can be done with simple atomics, it should not be a problem.

A serious issue is that if the sequence produces values in line, there will be eventually be stack overflow issues. This
can be worked-around by using a queue or scheduler to delay the next sender, but this is not ideal. There are ongoing
efforts to add tail senders to P2300, which would solve this issue but bring more complexity.

## Async RAII

In normal c++, RAII is used to ensure that resources are cleaned up when they go out of scope. However, this model
breaks down because of several limitations:

1. Asynchronous operations have 2 scopes: the lexical scope, and the "async" scope of the operation state.
2. Destructors cannot be asynchronous.
3. Destructors cannot return errors.

### The Async Call Stack

In the sender receiver model, operations do not start immediately. When a function returns a sender, all of its local
variables are destroyed, before the actual sender is started. Imagine the following sender function:

```cpp
auto my_sender() {
    // By the time this function returns, the thread pool will be destroyed, and the scheduler will be invalid.
    auto my_thread_pool = di::ThreadPool(4);
    return di::execution::on(my_thread_pool.get_scheduler(), do_some_work);
}
```

Even though sync RAII can cleanup all the threads in the thread pool, its lifetime does not match the lifetime of the
asynchronous operation. What's really happening is that the functions which create senders only create a "task graph"
which describes what work to do. We can not acquire the resources needed to do the work until the task graph is being
executed.

What we want to do instead is create a "node" in the task graph which acquires the resources, and reference that as part
of the sender. The actual thread pool object is now stored in the operation state, whose lifetime matches the lifetime
of the asynchronous operation.

### Cleanup can be Asynchronous

The second problem is that destructors cannot be asynchronous. This is a problem because the last thing an asynchronous
runtime wants to do is block on a synchronous operation. This holds even for the cleanup of resources. For instance, in
the example of the thread pool, we want to wait for all of the threads to finish before we can destroy the thread pool.
Technically, we can block in the destructor, but this shuts down all oppurtunities for parallelism. Other operations can
run while the threads are being joined, if only we could wait asynchronously. Additionally, we can imagine that if our
RAII objects were threads themselves, it would be much better to wait for all of the threads to finish asynchronously in
parallel whether than block on each thread one at a time.

### Cleanup can be Fallible

C++ destructors cannot return errors, which is inconvenient for asynchronous operations. For instance, it turns out that
calling join on a thread can fail spuriously (like the kernel is out of memory). Throwing an exception in a destructor
is very bad, and this project disables exceptions globally. The best we can do is ASSERT(false) in the destructor and
require the user to call `join()` manually. This is extremely terrible.

It gets worse when modelling things which are likely to fail, like network connections. When closing a network
connection, the operation can fail due to various reasons, like pending data not successfully getting flushed, dropping
packets, or an actively malicous peer. Being able to handle errors in these cases is important.

### Async RAII Working Design

The async RAII model is achieves these goals by modelling a resource as an async sequence which sends a single value
when the resource is acquired, and completes when the resource is released. The resource is acquired when the sequence's
first item completes, and the resource starts to be released when the sequence's receiver acknowledges the value. Then
the sequence completes when the resource is fully released.

This design idea is taken from the [async resource](https://kirkshoop.github.io/async_scope/async-resource.html) c++
proposal. The following code example shows a correct use of the async resource model:

```cpp
auto my_sender() {
    return di::execution::use_resources([](auto thread_pool_token) {
        return di::execution::on(thread_pool.get_scheduler(), do_some_work);
    }, di::make_deferred<di::ThreadPool>(4));
}
```

The `execution::use_resources()` function takes a function which returns a sender, and multiple deferred async
resources. In this model, the thread pool's construction is delayed until the sender is started. The sequence does not
send the thread pool itself, but instead sends a token which is essentially a reference-wrapper to the thread pool. This
is done so that the values can safely be decay copied, without ever moving the thread pool itself. The thread pool is
automatically joined after the returned sender completes.

### use_resources() Implementation

The implementation of `execution::use_resources()` is effectively a combination of a few sequence sender algorithms. It
is essentially as follows:

```cpp
auto use_resources(auto invocable, auto&&... deferred_resources) {
    return execution::let_value_with([invocable](auto& resources) {
        return execution::first_value(
            execution::zip(execution::run(resources)...)
                | let_value_each(invocable)
        );
    }, deferred_resources...);
}
```

The `execution::let_value_with()` algorithm allows us to consume the deferred resources by reference, after constructing
them in the operation state. The `execution::first_value()` algorithm is essentially a no-op, since the sequences only
send a single value, but is needed to convert the sequence into a sender. The `execution::zip()` algorithm is used to
combine the resources into a single sequence, which the invocable can consume.

The main interesting point is the `execution::run()` CPO, which is used to start the resource sequences. The
`execution::run()` CPO is the API which allows something to be an async resource. It takes an lvalue reference to the
resource, and returns a sequence sender, as described above.

### make_deferred Implementation

The `di::make_deferred()` function is used to create a deferred object. It is essentially defined as follows:

```cpp
template<typename T>
auto make_deferred(auto&& args) {
    return [args = di::forward<decltype(args)>(args)]() mutable {
        return T(di::move(args));
    };
}
```

The returned object is a function which can be invoked to construct the object. `T` can be immovable thanks to
guaranteed RVO. The `args` are moved into the function, so the returned function is copyable only if all `args` are
copyable.

Since this is the simple model for deferred objects, it is possible to provide arbitrary 0 argument lambdas as input to
the `execution::use_resources()` function if necessary. `di::make_deferred()` is just a convenience function to make
this easier.

### Async RAII in Coroutines

Coroutines allow asynchronous code to be written in c++ without the need for complex template metaprogramming and sender
algorithms. However, this async RAII model does not naturally fit into coroutines, unlike other concepts of the sender
receiver model. This is because the notion of an async scope does not mirror the lexical scope of a coroutine. For
instance:

```cpp
auto f() -> di::Lazy<> {
    for (int i = 0; i < 10; i++) {
        // Let's say this somehow worked.
        auto thread_pool = co_await di::co_use_resource<di::ThreadPool>(4);

        co_await di::execution::on(thread_pool.get_scheduler(), do_some_work);

        // At this point, we would expect the thread pool to be destroyed, but it cannot be. Async RAII in coroutines
        // would have to destroy the thread pool only when the coroutine itself is destroyed. So in effect, we would
        // have made 10 thread pools at once.
    }
}
```

The `co_use_resource()` function is a hypothetical coroutine version of `use_resources()`. The problem is that the
coroutine is a single node in the task graph, so there is no way to apply the async RAII model in a more granular way.
This is particular problematic if anyone tries to use an async mutex, since the mutex would be locked for the entire
coroutine. One could easily imagine this leading to deadlock, since the lock is not held for the lexical scope of the
variable. Additionally, it is unclear if `co_use_resource()` could even be implemented in a way which allows more that
one resource to be acquired.

The solution is to just call the `use_resources()` function from within the coroutine, and then immediately co_await the
result. This is not ideal, because it is more verbose, and importantly, is unworkable if we need to call `co_yield` in
the coroutine. These things can be probably be worked around, but it will be increasingly difficult to do so.

## Async Scope

A key aspect of structured concurrency is "scoping" groups of asynchronous operations into a single group. Before the
"outer" operation completes, all "inner" operations must complete. This additionally allows requesting cancellation of
the entire group of operations, and manages the memory and lifetime of the operations.

The library provides 3 CPOs for interacting with async scopes:

1. `execution::nest()`
2. `execution::spawn()`
3. `execution::spawn_future()`

### Nest

The `execution::nest()` CPO is used to wrap a sender in an async scope. It takes a sender, and returns a new sender
which signals the completion of the original sender to the async scope. This allows the scope to know when all of its
operations have completed.

### Spawn

The `execution::spawn()` CPO is used to spawn a task, without waiting for it to complete. It takes a sender, and runs it
in the provided async scope. The operation's lifetime is fully managed by the async scope, and it automatically deleted
when the operation completes. And like `execution::nest()`, it signals the completion of the operation to the async
scope so it can track when all operations have completed.

### Spawn Future

The `execution::spawn_future()` CPO is used to spawn a task, and return a future which has the values which the task
sends. It takes a sender, and runs it in the provided async scope. The operation's lifetime is fully managed by the
async scope, and it automatically deleted when the operation completes. And like `execution::nest()`, it signals the
completion of the operation to the async scope so it can track when all operations have completed. The difference
between this function and `execution::spawn()` is that this function returns a future which can be used to retrieve the
values, and thus requires more synchronization and memory overhead.

### Counting Scope

The `di::CountingScope` class template is a concrete implementation of an async scope. It is modelled as an async
resource, and can be used with the `execution::use_resources()` function. It is a counting scope, meaning it tracks the
number of operations which have been spawned, and signals completion when all operations have completed. Since it is an
async resource, this is done automatically when the resource is destroyed.

```cpp
auto spawn_sender = execution::use_resources(
    [&](auto scope, auto pool) {
        // Do 10 things in parallel on the thread pool.
        for (int i = 0; i < 10; i++) {
            execution::spawn(scope, execution::on(pool, do_some_work));
        }
        return execution::just();
    },
    di::make_deferred<di::CountingScope<>>(), di::make_deferred<di::ThreadPool>());
```

The async resource mechanism ensures that the waiting for the completion of the scope is done automatically, and also
enables a simple implementation.

### Benefits of Async Scope Abstraction

In the above example, the `CountingScope` is used to track the number of operations which have been spawned. This
enables spawning a dynamic number of operations, and waiting for them to complete. This is not possible with the
`execution::when_all()` function, since it is passed senders to wait for in parallel. This means the number of senders
must be known at compile time.

The async scope mechanism enables spawning tasks with regular loops and control flow, while still enabling the parallel
execution of the tasks, as well as waiting for them to complete. As such, it is a more natural fit for many use cases.

Some other examples where an async scope is useful:

- Spawning a task for each file in a directory, and waiting for them all to complete.
- Spawning a task for each request made to the Iris kernel, which scoping all operations to a specific process.
- Creating an async sequence which sends a values in parallel, and thus needs to spawn a task for each value.
- Conditionally spawning a task, depending on whether each value in a range is "interesting".

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
creates a problem, because the `di::connect` CPO is required to return a valid operation state, and this may require a
heap allocation.

#### Case 1: Creating the Type-Erased Receiver Fails

There is really no choice but to simply refuse to compile code if the type-erased receiver conversion is fallible. The
only potential alternative would be to immediately invoke the receiver with an error, but this could cause asyncrhonous
computations to start before the operation state is started, which breaks the entire model. Luckily, receivers can
always be implemented as storing a single pointer (to an operation state or stack variable), so this is not a problem.

#### Case 2: Creating the Type-Erased Sender Fails

This too is not really a problem. The `di::connect` CPO will be called with the type-erased sender already existing, so
there is no way for this to fail. However, creating the sender in the first place could fail. This implies that
functions returning a type-erased sender would have to return a `di::Result<di::AnySender<...>>`, which is not ideal,
especially since this model already encompasses errors. It would be a lot better to simply return a
`di::AnySender<...>`, and have the error be communicated through the operation state. This is possible, but it requires
making `di::Expected<Sender, E>` a valid sender, with completion signatures equivalent to `Sender` with the addition of
`di::SetError(E)`. This is not ideal, because it would also mean that this type would need some variant `connect`
function would return a variant operation state, which either holds the operation state of the sender, or the error.

A simpler approach is to add an implicit conversion between any valid `Sender` and `di::AnySender<...>`, which first
tries to create the type-erased sender, and if that fails, returns it instead returns `di::execution::just_error(E)`.
Since this error sender is simple, it can be created without heap allocation, and so the conversion function will always
return a valid sender.

#### Case 3: Creating the Type-Erased Operation State Fails

This is the most difficult case to deal with. The problem is that the `di::connect` CPO is required to return a valid
operation state, and may require a heap allocation. The only way to "fix" this is to make a dummy operation state, which
when started, immediately invokes the receiver with an error.

The good news is that this can be solved because the `di::connect` CPO is returning a type-erased operation state. Like
in the sender case, this can be done by adding a conversion function between `OperationState` and
`di::AnyOperationState<...>`. This function will first try to create the type-erased operation state, and if that fails,
it will return a dummy operation state which completes with an error once started.

One thing to worry about is that `OperationState` objects are not movable, so the library must take care to ensure that
copy-ellision is used. Additionally, the `di::AnyReceiver<...>` type will be move-only, so it cannot be stored in both
the original operation state and the dummy operation state. This is solvable using the fact that the creation of the
type-erased operation state only fails when allocating memory fails. This means that when trying to type-erase the
normal operation state, the library can first try to allocate memory for the type-erased receiver, and if that fails,
the actual `di::connect` function will never be called between the sender and receiver. Instead, the dummy operation
will be returned. This dummy operation state will be equivalent to the result of connecting
`di::execution::just_error(E)` to the type-erased receiver.

The current implementation requires type-erased operation states to be movable to be stored in the inline storage, which
is very unfortunate. This is because c++ does not guarantee copy-elision of named return values. As a consequence, the
current implementation, which uses the `emplace()` method of `di::Any` to perform this two-pass construction, requires
the type to be movable. This is really bad, since it greatly increases the number of heap allocations required. The good
news is that this decision is transparent to users of the library, since heap allocation failures are already handled
transparently by resulting in an error.

This limitation can be resolved by either waiting for the standard to guarantee NRVO, or by using a work-around
two-phase construction mechanism that only relies on RVO. This would work by having a static method of `di::Any` which
creates some sort of token object, which proves the required memory is allocated and so construction can be infallible.
Another approach that could be considered is creating dummy move operations which assert that they are never called. If
the compiler is going to perform NRVO anyway, then these dummy move operations will never be called (maybe...), and
since operation states are internal to the library, this might even be safe. This is the simplest approach although it
is probably the worst idea in terms of safety.

## References

- [P2300 - std::execution](https://wg21.link/p2300)
- [P2300 Reference Implementation](https://github.com/NVIDIA/stdexec)
- [Sequence Senders](https://github.com/kirkshoop/sequence-next)
- [Async Resources](https://kirkshoop.github.io/async_scope/async-resource.html)
- [libunifex](https://github.com/facebookexperimental/libunifex/)
- [NJS's Blog on Structured
  Concurrency](https://vorpus.org/blog/notes-on-structured-concurrency-or-go-statement-considered-harmful/)

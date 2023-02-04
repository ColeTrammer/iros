# Type Erasure

Down with virtual methods.

## Traditional OOP

In normal C++, you would use a virtual interface to enable runtime polymorphism.

```c++
class IDrawable {
public:
    virtual IDrawable() = 0;

    virtual void draw() = 0;
};

class Square : public IDrawable {
public:
    virtual void draw() override {
        std::println("Draw square.");
    }
};

class Circle : public IDrawable {
public:
    virtual void draw() override {
        std::println("Draw circle.");
    }
};
```

Now, to use this construction, you can pass a IDrawable by reference to a function. To actually store these things in a memory safe way, either std::unique_ptr or std::shared_ptr must be used. These types cannot be treated as values, so we must use indirection. Furthremore, the objects always have to be heap allocated.

## Type Erasure

Using type erasure, the interface class poses an abstract set of requirements, and can be constructed from any type which meets them. The type internally is memory safe, by either storing the object internally (small object optimization), or on the heap using a smart pointer. Instead of using 2 indirections, the virtual table can be inline or otherwise stored using a "fat" pointer (rust dyn&).

```c++
class Circle {};
class Sqaure {};

struct Draw {
    template<typename T>
    requires(di::concepts::TagInvocable<DrawFunction, T&>)
    void operator()(T& object) const {
        function::tag_invoke(*this, object);
    }
};

constexpr inline auto draw = detail::DrawFunction {};

using AnyDrawable = di::AnyValue<
    Draw(di::Self&)
>;

void tag_invoke(Draw, Circle& self) {
    std::println("Draw circle.");
}

void tag_invoke(Draw, Square& self) {
    std::println("Draw square.");
}

void use_circle() {
    auto drawable = AnyDrawable(Circle {});
    draw(drawable);
}

void use_drawables() {
    auto drawables = di::Vector<AnyDrawable> {};
    drawables.emplace_back(Circle {});
    drawables.emplace_back(Sqaure {});

    di::for_each(drawables, draw);
}
```

Notice, drawables can be used directly as objects, and thus don't have to managed using smart pointers. Additionally, operations need to be defined inside the classes they operate on, which means Circle and Square can be pure data classes, and offer no functionality themselves. This allows seamlessly adding new operations without breaking code.

Default operations can be expressed directly in the definition of the operation. By providing such a default operation, the Draw function object will be invocable for any object, and thus every object can be erased into a drawable.

The tag_invoke mechanism allows type erasure without macros and without defining operations twice. However, you don't get member functions, since C++ does not have reflection and meta classes. However, not using member functions allows extending a class without modifying it, and still provides a uniform and readable way to call methods.

```c++
// OOP
object->draw();

// Type erasure
draw(object);
```

Calling a free function is 2 characters shorter assuming we are already in the correct namespace, although otherwise it will be more characters to type.

## Ergonomic Concerns

The main annoyance with this model is the creation of tag_invoke calling function objects. This can actually be automated using some meta programming on top of tag_invoke().

```c++
// OOP
class IDrawable {
public:
    virtual ~IDrawable() = 0;

    virtual void draw() = 0;
    virtual i32 get_area() const = 0;
    virtual void debug_print() const {}
};

// Type erasure.
struct Draw : di::Method<Draw, void(di::Self&)> {};
struct GetArea : di::Method<GetArea, i32(di::Self const&)> {};
struct DebugPrint : di::Method<DebugPrint,
    void(di::Self const&),
    di::Nontype<di::into_void>
> {};

constexpr inline auto draw = Draw {};
constexpr inline auto get_area = GetArea {};
constexpr inline auto debug_print = DebugPrint {};

using IDrawable = di::meta::List<
    Draw, GetArea, DebugPrint
>;

using Drawable = di::AnyValue<IDrawable>;
using DrawableRef = di::AnyRef<IDrawable>;
```

The idea is that the dispatch objects will implement the common CPO pattern, which is to attempt to call functions one after another. For DebugPrint, the final function object to call is di::into_void, which means that the default implenentation will just ignore arguments. This DSL for describing an interface can work without macros, and is in fact far more expressive than virtual methods.

### Templated Dispatch

Most type erased interfaces will be designed for type erasure, which means they will not have templated arguments. But, in other cases, the interface in question will be templated. This is the case for things like sender/receivers, function objects, di::format, etc. In these cases, an explicit signature must be provided. For di::format, a specific type erased format context will be used, or for di::Function, the type erasure can only work on callables that except the provided signature.

For these cases, there needs to be a way to explicitly list the signature when defining the interface requirements.

```c++
using Interface = di::meta::List<
    Method1, Method2,
    void(FunctionObject, di::Self&),
    i32(OtherFunction, di::Self const&, i32)
>;
```

If we really wanted, it could be possible to overload operator-> on some sort of type. The interface definition would then be a list of value types:

```c++
using Interface = di::meta::ValueList<
    method1, method2,
    di::member<FunctionObject(di::Self&)> -> di::InPlaceType<void>,
    di::member<OtherFunction(di::Self const&, i32)> -> di::InPlaceType<i32>
>;
```

This is more verbose an exotic, so probably won't be way to go.

### Expressivity for complex CPOs

The dispatcher API can also be used to make defining more complicated CPOs a lot more bearable.

The current implementation of container::begin() is as follows:

struct BeginFunction;

```c++
namespace detail {
    template<typename T>
    concept ArrayBegin = concepts::LanguageArray<meta::RemoveReference<T>>;

    template<typename T>
    concept CustomBegin = concepts::TagInvocable<BeginFunction, T> &&
                          concepts::Iterator<meta::Decay<meta::TagInvokeResult<BeginFunction, T>>>;

    template<typename T>
    concept MemberBegin = requires(T&& container) {
                              { util::forward<T>(container).begin() } -> concepts::Iterator;
                          };
}

struct BeginFunction {
    template<typename T>
    requires(enable_borrowed_container(types::in_place_type<meta::RemoveCV<T>>) &&
             (detail::ArrayBegin<T> || detail::CustomBegin<T> || detail::MemberBegin<T>) )
    constexpr auto operator()(T&& container) const {
        if constexpr (detail::ArrayBegin<T>) {
            return container + 0;
        } else if constexpr (detail::CustomBegin<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else {
            return util::forward<T>(container).begin();
        }
    }
};
```

This can equivalently be written as:

```c++
namespace detail {
struct BeginArray {
    template<typename T>
    requires(concepts::LanguageArray<meta::RemoveReference<T>>)
    constexpr auto operator()(T&& array) const {
        return array + 0;
    }
};

struct BeginMember {
    template<typename T>
    constexpr auto operator()(T&& container) const
    requires(requires {
        { util::forward<T>(container).begin() } -> concepts::Iterator;
    })
    {
        return util::forward<T>(container).begin();
    }
};

struct BeginFunction : Dipatcher<BeginFunction, meta::List<
    BeginArray(meta::_1),
    TagInvoke(meta::Self&, meta::_1),
    BeginMemer(meta::_1)
>> {};
}
```

## Multiple types of erased objects

So far, only a value oriented type erased object has been considered. But we can additionally have a type erased reference type, with non-owning semantics, which can be thought of as equivalent to passing a dyn& in rust. This type
would be 100% allocation free.

Additionally, one could imagine creating a type erased wrapper
which internally has shared pointer semantics, such that it owns its data and has shallow copy semantics.

## Implementation

### Object Management

For the value oriented erased object, there are many considerations to be made, mainly around which operations are to be supported. If a type erased wrapper supports copying, all implementations must support copying as well. The same can be said for di CPOs, like di::clone. Making the erased object trivially relocatable greatly improves performance, because indirect calls can be ellided during move construction, but this requires all implementing types to be trivially relocatable themselves. This is mainly a problem because this information is not derived in the compiler (at least for GCC), so such a property is opt-in.

No indirection on moving is needed if the internal object is always heap allocated, but doing so could be wasteful. Having inline storage is very important when erasing small objects (imagine di::Function), but effectively useless if every object is large (imagine iris::IrqController). As such, the internal storage policy needs to be heavily customizable.

### Virtual Table Storage

Manually creating a vtable enables the programmer to micro-optimize the vtable layout as much as they please. A sensible default is to store the vtable as a "fat" pointer (separate pointer to array of function pointers), but if there is only 1 operation, it is obviously better to just store that function pointer directly. Since we will always need at least 1 operation, because the destructor must always be callable, we can expand the default inlining level to 2 or 3 operations.

In certain cases, one function is "hot" while the other erased functions are called much less frequently. In these scenarios, a hybrid approach should offer the best performance. This is again an area which requires extreme customizability.

### Meta Object Representation

To store entries in the vtable, we need compile time meta programming facilities. Vtable entries will be represented in the following structure.

```c++
namespace meta {
// Usage: Signature<MyFunction, void(di::Self&)>
template<typename T, concepts::LanguageFunction S>
struct Signature {
    using Type = S;
    using Tag = T;
};
}

namespace concepts {
template<typename T>
concept Signature = InstanceOf<T, meta::Signature>;
}

namespace meta {
template<concepts::Signature Sig>
using SignatureTag = Sig::Tag;

template<concepts::Signature Sig>
using SignatureType = meta::Type<Sig>;
}
```

Then, vtables will have an associated list of signature objects, which correspond to the vtable entries.
The library will support merging vtables together, to enable erasing multiple traits into one object, and
as an implementation detail, because owning structures will internally merge the user requested operations
with the vtable for moving, destroying, copying, swapping, etc.

#pragma once

#include <app/widget_bridge_interface.h>
#include <eventloop/forward.h>
#include <graphics/point.h>
#include <liim/maybe.h>

#define APP_WIDGET_BASE_IMPL(BaseType, ParentType, Self, ...)                                  \
public:                                                                                        \
    using BaseWidget = BaseType;                                                               \
                                                                                               \
protected:                                                                                     \
    static SharedPtr<BaseType> create_base_object(App::Object* object, SharedPtr<Self> self) { \
        return BaseWidget::create_without_initializing(object, ##__VA_ARGS__);                 \
    }                                                                                          \
                                                                                               \
public:                                                                                        \
    BaseWidget& base() { return typed_object<BaseWidget>(); }                                  \
    const BaseWidget& base() const { return typed_object<BaseWidget>(); }                      \
                                                                                               \
private:

#define APP_WIDGET_IMPL_NO_BASE(ParentType)    \
public:                                        \
    using BaseWidget = ParentType::BaseWidget; \
                                               \
private:

#define APP_WIDGET_IMPL(ParentType, Self)                                                                            \
public:                                                                                                              \
    using ParentWidget = ParentType;                                                                                 \
                                                                                                                     \
    Self(const Self&) = delete;                                                                                      \
    Self(Self&&) = delete;                                                                                           \
                                                                                                                     \
    template<typename... Args>                                                                                       \
    static App::WidgetCreationResultOwned<BaseWidget, Self> create_both_owned(App::Object* parent, Args&&... args) { \
        auto custom = make_shared<Self>(forward<Args>(args)...);                                                     \
        auto base = create_base_object(parent, custom);                                                              \
        custom->attach(*base);                                                                                       \
        base->initialize();                                                                                          \
        return { base, custom };                                                                                     \
    }                                                                                                                \
                                                                                                                     \
    template<typename... Args>                                                                                       \
    static SharedPtr<BaseWidget> create_base_owned(App::Object* parent, Args&&... args) {                            \
        return create_both_owned(parent, forward<Args>(args)...).base;                                               \
    }                                                                                                                \
                                                                                                                     \
    template<typename... Args>                                                                                       \
    static SharedPtr<Self> create_owned(App::Object* parent, Args&&... args) {                                       \
        assert(parent);                                                                                              \
        return create_both_owned(parent, forward<Args>(args)...).custom;                                             \
    }                                                                                                                \
                                                                                                                     \
    template<typename... Args>                                                                                       \
    static App::WidgetCreationResultRef<BaseWidget, Self> create_both(App::Object* parent, Args&&... args) {         \
        return *create_both_owned(parent, forward<Args>(args)...);                                                   \
    }                                                                                                                \
                                                                                                                     \
    template<typename... Args>                                                                                       \
    static BaseWidget& create_base(App::Object* parent, Args&&... args) {                                            \
        return *create_both_owned(parent, forward<Args>(args)...).base;                                              \
    }                                                                                                                \
                                                                                                                     \
    template<typename... Args>                                                                                       \
    static Self& create(App::Object* parent, Args&&... args) {                                                       \
        return *create_both_owned(parent, forward<Args>(args)...).custom;                                            \
    }                                                                                                                \
                                                                                                                     \
private:

#define APP_WIDGET_EMITS_IMPL(...)                                                                                                     \
public:                                                                                                                                \
    template<typename... Ev>                                                                                                           \
    static constexpr bool does_emit() {                                                                                                \
        return (LIIM::IsOneOf<Ev, ##__VA_ARGS__>::value && ...) || ParentWidget::does_emit<Ev...>() || BaseWidget::does_emit<Ev...>(); \
    }                                                                                                                                  \
                                                                                                                                       \
private:

#define APP_WIDGET_BASE(BaseType, ParentType, Self, ...)            \
    APP_WIDGET_BASE_IMPL(BaseType, ParentType, Self, ##__VA_ARGS__) \
    APP_WIDGET_IMPL(ParentType, Self)                               \
    APP_OBJECT_FORWARD_EVENT_API(base())                            \
    APP_WIDGET_EMITS_IMPL()

#define APP_WIDGET_BASE_EMITS(BaseType, ParentType, Self, Emits, ...) \
    APP_WIDGET_BASE_IMPL(BaseType, ParentType, Self, ##__VA_ARGS__)   \
    APP_WIDGET_IMPL(ParentType, Self)                                 \
    APP_OBJECT_FORWARD_EVENT_API(base())                              \
    APP_WIDGET_EMITS_IMPL EMITS

#define APP_WIDGET(ParentType, Self)     \
    APP_WIDGET_IMPL_NO_BASE(ParentType)  \
    APP_WIDGET_IMPL(ParentType, Self)    \
    APP_OBJECT_FORWARD_EVENT_API(base()) \
    APP_WIDGET_EMITS_IMPL()

#define APP_WIDGET_EMITS(ParentType, Self, Emits) \
    APP_WIDGET_IMPL_NO_BASE(ParentType)           \
    APP_WIDGET_IMPL(ParentType, Self)             \
    APP_OBJECT_FORWARD_EVENT_API(base())          \
    APP_WIDGET_EMITS_IMPL Emits

namespace App {
template<typename BaseWidget, typename CustomWidget>
struct WidgetCreationResultRef {
    BaseWidget& base;
    CustomWidget& custom;
};

template<typename BaseWidget, typename CustomWidget>
struct WidgetCreationResultOwned {
    SharedPtr<BaseWidget> base;
    SharedPtr<CustomWidget> custom;

    WidgetCreationResultRef<BaseWidget, CustomWidget> operator*() { return { *base, *custom }; }
};

class WidgetBridge {
public:
    template<typename... Ev>
    static constexpr bool does_emit() {
        return false;
    }

    virtual ~WidgetBridge() {}

    // iros reflect begin
    virtual bool steals_focus() { return false; }

    virtual Maybe<Point> cursor_position() { return {}; }
    // iros reflect end

    virtual void render() {}

private:
    Object* m_object { nullptr };
};
}

#pragma once

#include <liim/preprocessor.h>

#define __APP_EVENT_PARAM(type, name)       type name
#define __APP_EVENT_MOVE_NAME(type, name)   move(name)
#define __APP_EVENT_MEMBER_INIT(type, name) m_##name(move(name))

#define __APP_EVENT_PARAMATER_LIST(acc, field)     LIIM_LIST_APPEND(acc, __APP_EVENT_PARAM field)
#define __APP_EVENT_NAME_LIST(acc, field)          LIIM_LIST_APPEND(acc, __APP_EVENT_MOVE_NAME field)
#define __APP_EVENT_MEMBER_INITIALIZER(acc, field) LIIM_LIST_APPEND(acc, __APP_EVENT_MEMBER_INIT field)
#define __APP_EVENT_MEMBER(type, name)             type m_##name;

// clang-format off
#define __APP_EVENT_CONSTRUCTOR(EventName, Base, base_fields, fields, is_parent)                                \
    explicit EventName(                                                                                         \
                LIIM_LIST_JOIN_COMMA(                                                                           \
                    LIIM_LIST_CONCAT(                                                                           \
                        LIIM_EVAL(LIIM_LIST_REDUCE(__APP_EVENT_PARAMATER_LIST, LIIM_LIST_INIT(), base_fields)), \
                        LIIM_EVAL(LIIM_LIST_REDUCE(__APP_EVENT_PARAMATER_LIST, LIIM_LIST_INIT(), fields))))     \
             )                                                                                                  \
        : Base(                                                                                                 \
            LIIM_LIST_JOIN_COMMA(                                                                               \
                LIIM_IF(is_parent)                                                                              \
                    (LIIM_EVAL(LIIM_LIST_REDUCE(__APP_EVENT_NAME_LIST, LIIM_LIST_INIT(), base_fields)))         \
                    (LIIM_LIST_PREPEND(                                                                         \
                        LIIM_EVAL(LIIM_LIST_REDUCE(__APP_EVENT_NAME_LIST, LIIM_LIST_INIT(), base_fields)),      \
                        static_event_name())))                                                                  \
          )                                                                                                     \
                                                                                                                \
            LIIM_COMMA_IF(LIIM_LIST_NOT_EMPTY(fields))                                                          \
                                                                                                                \
            LIIM_LIST_JOIN_COMMA(                                                                               \
                LIIM_EVAL(LIIM_LIST_REDUCE(__APP_EVENT_MEMBER_INITIALIZER, LIIM_LIST_INIT(), fields))) {}
// clang-format on

#define __APP_EVENT_GETTER(type, name) \
    const type& name() const { return m_##name; }
#define __APP_EVENT_GETTERS(fields) LIIM_EVAL(LIIM_LIST_FOR_EACH(__APP_EVENT_GETTER, fields))

#define __APP_EVENT_SETTER(type, name) \
    void set_##name(type name) { m_##name = move(name); }
#define __APP_EVENT_SETTERS(fields) LIIM_EVAL(LIIM_LIST_FOR_EACH(__APP_EVENT_SETTER, fields))

#define __APP_EVENT_METHODS(methods) LIIM_EVAL(LIIM_LIST_FOR_EACH(LIIM_ID, methods))

#define __APP_EVENT_MEMBERS(fields) LIIM_EVAL(LIIM_LIST_FOR_EACH(__APP_EVENT_MEMBER, fields))

#define APP_EVENT_IMPL(Namespace, EventName, Base, base_fields, fields, methods, requires_handling, is_parent) \
    namespace Namespace {                                                                                      \
    class EventName : public Base {                                                                            \
        APP_EVENT_HEADER_IMPL(Namespace, EventName, requires_handling)                                         \
                                                                                                               \
    public:                                                                                                    \
        __APP_EVENT_CONSTRUCTOR(EventName, Base, base_fields, fields, is_parent)                               \
        __APP_EVENT_GETTERS(fields)                                                                            \
        __APP_EVENT_SETTERS(fields)                                                                            \
        __APP_EVENT_METHODS(methods)                                                                           \
                                                                                                               \
    private:                                                                                                   \
        __APP_EVENT_MEMBERS(fields)                                                                            \
    };                                                                                                         \
    }

#define APP_EVENT_HEADER_IMPL(Namespace, EventName, requires_handling)                        \
public:                                                                                       \
    static constexpr StringView static_event_name() { return "" #Namespace "::" #EventName; } \
    static constexpr bool event_requires_handling() { return requires_handling; }             \
                                                                                              \
private:

#define APP_EVENT_HEADER_REQUIRES_HANDLING(Namespace, EventName) APP_EVENT_HEADER_IMPL(Namespace, EventName, true)

#define APP_EVENT_HEADER(Namespace, EventName) APP_EVENT_HEADER_IMPL(Namespace, EventName, false)

#define APP_EVENT(Namespace, EventName, Base, base_fields, fields, methods) \
    APP_EVENT_IMPL(Namespace, EventName, Base, base_fields, fields, methods, 0, 0)

#define APP_EVENT_PARENT(Namespace, EventName, Base, base_fields, fields, methods) \
    APP_EVENT_IMPL(Namespace, EventName, Base, base_fields, fields, methods, false, 1)

#define APP_EVENT_REQUIRES_HANDLING(Namespace, EventName, Base, base_fields, fields, methods) \
    APP_EVENT_IMPL(Namespace, EventName, Base, base_fields, fields, methods, 1, 0)

#define APP_EVENT_PARENT_REQUIRES_HANDLING(Namespace, EventName, Base, base_fields, fields, methods) \
    APP_EVENT_IMPL(Namespace, EventName, Base, base_fields, fields, methods, 1, 1)

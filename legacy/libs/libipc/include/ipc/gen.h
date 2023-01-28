#pragma once

#include <ipc/endpoint.h>
#include <ipc/message.h>
#include <ipc/message_dispatcher.h>
#include <ipc/stream.h>
#include <liim/preprocessor.h>

#define __MESSAGE_FIELD_DECL(type, name)               type name;
#define __MESSAGE_FIELD_SERIALIZATION_SIZE(type, name) ret += IPC::Serializer<type>::serialization_size(this->name);
#define __MESSAGE_FIELD_SERIALIZER(type, name)         stream << this->name;
#define __MESSAGE_FIELD_DESERIALIZER(type, name)       stream >> this->name;

#define __MESSAGE_DECL(...)                    LIIM_FOR_EACH(__MESSAGE_FIELD_DECL, ##__VA_ARGS__)
#define __MESSAGE_SERIALIZATION_SIZE_BODY(...) LIIM_FOR_EACH(__MESSAGE_FIELD_SERIALIZATION_SIZE, ##__VA_ARGS__)
#define __MESSAGE_SERIALIZER_BODY(...)         LIIM_FOR_EACH(__MESSAGE_FIELD_SERIALIZER, ##__VA_ARGS__)
#define __MESSAGE_SERIALIZER(n, ...)                   \
    uint32_t serialization_size() const {              \
        uint32_t ret = 2 * sizeof(uint32_t);           \
        __MESSAGE_SERIALIZATION_SIZE_BODY(__VA_ARGS__) \
        return ret;                                    \
    }                                                  \
    bool serialize(IPC::Stream& stream) const {        \
        stream << serialization_size();                \
        stream << static_cast<uint32_t>(Type::n);      \
        __MESSAGE_SERIALIZER_BODY(__VA_ARGS__)         \
        return !stream.error();                        \
    }

#define __MESSAGE_DESERIALIZER_BODY(...) LIIM_FOR_EACH(__MESSAGE_FIELD_DESERIALIZER, ##__VA_ARGS__)
#define __MESSAGE_DESERIALIZER(n, ...)                \
    bool deserialize(IPC::Stream& stream) {           \
        uint32_t size;                                \
        uint32_t type;                                \
        stream >> size;                               \
        stream >> type;                               \
        if (type != static_cast<uint32_t>(Type::n)) { \
            return false;                             \
        }                                             \
        __MESSAGE_DESERIALIZER_BODY(__VA_ARGS__)      \
        if (size != serialization_size()) {           \
            return false;                             \
        }                                             \
        return !stream.error();                       \
    }

#define __MESSAGE_BEGIN(n) \
    struct n {             \
        static constexpr Type message_type() { return Type::n; }
#define __MESSAGE_BODY(n, ...) __MESSAGE_DECL(__VA_ARGS__) __MESSAGE_SERIALIZER(n, ##__VA_ARGS__) __MESSAGE_DESERIALIZER(n, ##__VA_ARGS__)
#define __MESSAGE_END(n) \
    }                    \
    ;                    \
    static_assert(IPC::ConcreteMessage<n>);

#define __MESSAGE_TYPE(n, ...) n,
#define __MESSAGE(n, ...)          \
    __MESSAGE_BEGIN(n)             \
    __MESSAGE_BODY(n, __VA_ARGS__) \
    __MESSAGE_END(n)

#define __DISPATCHER_DECL(n, ...) virtual void handle(IPC::Endpoint&, const n&) = 0;
#define __DISPATCHER(n, ...)            \
    case Type::n: {                     \
        n val;                          \
        if (!val.deserialize(stream)) { \
            handle_error(endpoint);     \
            return;                     \
        }                               \
        handle(endpoint, val);          \
        break;                          \
    }

#define __IPC_MESSAGE_TYPES(...)                                   \
    enum class Type : uint32_t {                                   \
        LIIM_FOR_EACH(__MESSAGE_TYPE, ##__VA_ARGS__) MessageCount, \
    };
#define __IPC_MESSAGES(...) LIIM_FOR_EACH(__MESSAGE, ##__VA_ARGS__)
#define __IPC_DISPATCHER(...)                                                                   \
    class MessageDispatcher : public IPC::MessageDispatcher {                                   \
    public:                                                                                     \
        LIIM_FOR_EACH(__DISPATCHER_DECL, ##__VA_ARGS__)                                         \
        virtual void handle_incoming_data(IPC::Endpoint& endpoint, IPC::Stream& stream) final { \
            uint32_t size;                                                                      \
            uint32_t type;                                                                      \
            stream >> size;                                                                     \
            stream >> type;                                                                     \
            if (stream.error()) {                                                               \
                handle_error(endpoint);                                                         \
                return;                                                                         \
            }                                                                                   \
            if (size != stream.buffer_max()) {                                                  \
                handle_error(endpoint);                                                         \
                return;                                                                         \
            }                                                                                   \
            stream.rewind();                                                                    \
            switch (static_cast<Type>(type)) {                                                  \
                LIIM_FOR_EACH(__DISPATCHER, ##__VA_ARGS__)                                      \
                default:                                                                        \
                    handle_error(endpoint);                                                     \
                    return;                                                                     \
            }                                                                                   \
        }                                                                                       \
    };

#define IPC_MESSAGES(n, ...)                    \
    namespace n {                               \
    LIIM_EVAL(__IPC_MESSAGE_TYPES(__VA_ARGS__)) \
    LIIM_EVAL(__IPC_MESSAGES(__VA_ARGS__))      \
    LIIM_EVAL(__IPC_DISPATCHER(__VA_ARGS__))    \
    }

#pragma once

#include <liim/pointers.h>
#include <liim/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/hal/input.h>

typedef uint64_t wid_t;

namespace WindowServer {

struct Message {
    enum class Type {
        Invalid,
        CreateWindowRequest,
        CreateWindowResponse,
        RemoveWindowRequest,
        RemoveWindowResponse,
        SwapBufferRequest,
        KeyEventMessage,
        MouseEventMessage,
    };

    struct CreateWindowRequest {
        static SharedPtr<Message> create(int x, int y, int width, int height, const String& name) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(CreateWindowRequest) + name.size() + 1);
            message->type = Message::Type::CreateWindowRequest;
            message->data_len = sizeof(CreateWindowRequest) + name.size() + 1;
            auto& request = message->data.create_window_request;
            request.x = x;
            request.y = y;
            request.width = width;
            request.height = height;
            strcpy(request.name, name.string());
            return SharedPtr<Message>(message);
        }

        int x;
        int y;
        int width;
        int height;
        char name[0];
    };

    struct CreateWindowResponse {
        static SharedPtr<Message> create(wid_t id, size_t size, const char* path) {
            size_t path_len = strlen(path);
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(CreateWindowResponse) + path_len + 1);
            message->type = Message::Type::CreateWindowResponse;
            message->data_len = sizeof(CreateWindowResponse) + path_len + 1;
            auto& response = message->data.create_window_response;
            response.window_id = id;
            response.shared_buffer_size = size;
            strcpy(response.shared_buffer_path, path);
            return SharedPtr<Message>(message);
        }

        wid_t window_id;
        size_t shared_buffer_size;
        char shared_buffer_path[0];
    };

    struct RemoveWindowRequest {
        static SharedPtr<Message> create(wid_t id) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(RemoveWindowRequest));
            message->type = Message::Type::RemoveWindowRequest;
            message->data_len = sizeof(RemoveWindowRequest);
            auto& request = message->data.remove_window_request;
            request.wid = id;
            return SharedPtr<Message>(message);
        }

        wid_t wid;
    };

    struct RemoveWindowResponse {
        static SharedPtr<Message> create(bool success) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(RemoveWindowResponse));
            message->type = Message::Type::RemoveWindowResponse;
            message->data_len = sizeof(RemoveWindowResponse);
            auto& response = message->data.remove_window_response;
            response.success = success;
            return SharedPtr<Message>(message);
        }

        bool success;
    };

    struct SwapBufferRequest {
        static SharedPtr<Message> create(wid_t wid) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(SwapBufferRequest));
            message->type = Message::Type::SwapBufferRequest;
            message->data_len = sizeof(SwapBufferRequest);
            auto& request = message->data.swap_buffer_request;
            request.wid = wid;
            return SharedPtr<Message>(message);
        }

        wid_t wid;
    };

    struct KeyEventMessage {
        static UniquePtr<Message> create(key_event event) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(KeyEventMessage));
            message->type = Message::Type::KeyEventMessage;
            message->data_len = sizeof(KeyEventMessage);
            auto& data = message->data.key_event_message;
            data.event = event;
            return UniquePtr<Message>(message);
        }

        key_event event;
    };

    struct MouseEventMessage {
        static UniquePtr<Message> create(mouse_event event) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(MouseEventMessage));
            message->type = Message::Type::MouseEventMessage;
            message->data_len = sizeof(MouseEventMessage);
            auto& data = message->data.mouse_event_message;
            data.event = event;
            return UniquePtr<Message>(message);
        }

        mouse_event event;
    };

    size_t total_size() const { return sizeof(Message) + data_len; };

    Type type { Type::Invalid };
    int data_len { 0 };
    union {
        CreateWindowRequest create_window_request;
        CreateWindowResponse create_window_response;
        RemoveWindowRequest remove_window_request;
        RemoveWindowResponse remove_window_response;
        SwapBufferRequest swap_buffer_request;
        KeyEventMessage key_event_message;
        MouseEventMessage mouse_event_message;
    } data;
};

};
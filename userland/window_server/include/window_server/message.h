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

enum class WindowType {
    Application,
    Frameless,
};

struct Message {
    enum class Type {
        Invalid,
        CreateWindowRequest,
        CreateWindowResponse,
        RemoveWindowRequest,
        RemoveWindowResponse,
        ChangeWindowVisibilityRequest,
        ChangeWindowVisibilityResponse,
        SwapBufferRequest,
        KeyEventMessage,
        MouseEventMessage,
        WindowDidResizeMessage,
        WindowReadyToResizeMessage,
        WindowReadyToResizeResponse,
        WindowClosedEventMessage,
        WindowRenameRequest,
    };

    struct CreateWindowRequest {
        static SharedPtr<Message> create(int x, int y, int width, int height, const String& name, WindowType type) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(CreateWindowRequest) + name.size() + 1);
            message->type = Message::Type::CreateWindowRequest;
            message->data_len = sizeof(CreateWindowRequest) + name.size() + 1;
            auto& request = message->data.create_window_request;
            request.x = x;
            request.y = y;
            request.width = width;
            request.height = height;
            request.type = type;
            strcpy(request.name, name.string());
            return SharedPtr<Message>(message);
        }

        int x;
        int y;
        int width;
        int height;
        WindowType type;
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

    struct ChangeWindowVisibilityRequeset {
        static SharedPtr<Message> create(wid_t wid, bool visible) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(ChangeWindowVisibilityRequeset));
            message->type = Message::Type::ChangeWindowVisibilityRequest;
            message->data_len = sizeof(ChangeWindowVisibilityRequeset);
            auto& request = message->data.change_window_visibility_request;
            request.wid = wid;
            request.visible = visible;
            return SharedPtr<Message>(message);
        }

        wid_t wid;
        bool visible;
    };

    struct ChangeWindowVisibilityResponse {
        static SharedPtr<Message> create(wid_t wid, bool visible) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(ChangeWindowVisibilityResponse));
            message->type = Message::Type::ChangeWindowVisibilityResponse;
            message->data_len = sizeof(ChangeWindowVisibilityResponse);
            auto& response = message->data.change_window_visibility_response;
            response.wid = wid;
            response.visible = visible;
            return SharedPtr<Message>(message);
        }

        wid_t wid;
        bool visible;
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
        static UniquePtr<Message> create(wid_t wid, key_event event) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(KeyEventMessage));
            message->type = Message::Type::KeyEventMessage;
            message->data_len = sizeof(KeyEventMessage);
            auto& data = message->data.key_event_message;
            data.wid = wid;
            data.event = event;
            return UniquePtr<Message>(message);
        }

        wid_t wid;
        key_event event;
    };

    struct MouseEventMessage {
        static UniquePtr<Message> create(wid_t wid, int x, int y, scroll_state scroll, mouse_button_state left, mouse_button_state right) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(MouseEventMessage));
            message->type = Message::Type::MouseEventMessage;
            message->data_len = sizeof(MouseEventMessage);
            auto& data = message->data.mouse_event_message;
            data.wid = wid;
            data.x = x;
            data.y = y;
            data.scroll = scroll;
            data.left = left;
            data.right = right;
            return UniquePtr<Message>(message);
        }

        wid_t wid;
        int x;
        int y;
        scroll_state scroll;
        mouse_button_state left;
        mouse_button_state right;
    };

    struct WindowDidResizeMessage {
        static SharedPtr<Message> create(wid_t wid) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(WindowDidResizeMessage));
            message->type = Message::Type::WindowDidResizeMessage;
            message->data_len = sizeof(WindowDidResizeMessage);
            auto& request = message->data.window_did_resize_message;
            request.wid = wid;
            return SharedPtr<Message>(message);
        }

        wid_t wid;
    };

    struct WindowReadyToResizeMessage {
        static SharedPtr<Message> create(wid_t wid) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(WindowReadyToResizeMessage));
            message->type = Message::Type::WindowReadyToResizeMessage;
            message->data_len = sizeof(WindowReadyToResizeMessage);
            auto& request = message->data.window_ready_to_resize_message;
            request.wid = wid;
            return SharedPtr<Message>(message);
        }

        wid_t wid;
    };

    struct WindowReadyToResizeResponse {
        static UniquePtr<Message> create(wid_t wid, int new_width, int new_height) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(WindowReadyToResizeResponse));
            message->type = Message::Type::WindowReadyToResizeResponse;
            message->data_len = sizeof(WindowReadyToResizeResponse);
            auto& data = message->data.window_ready_to_resize_response;
            data.wid = wid;
            data.new_width = new_width;
            data.new_height = new_height;
            return UniquePtr<Message>(message);
        }

        wid_t wid;
        int new_width;
        int new_height;
    };

    struct WindowClosedEventMessage {
        static UniquePtr<Message> create(wid_t wid) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(MouseEventMessage));
            message->type = Message::Type::WindowClosedEventMessage;
            message->data_len = sizeof(WindowClosedEventMessage);
            auto& data = message->data.window_closed_event_messasge;
            data.wid = wid;
            return UniquePtr<Message>(message);
        }

        wid_t wid;
    };

    struct WindowRenameRequest {
        static SharedPtr<Message> create(wid_t wid, const String& name) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(WindowRenameRequest) + name.size() + 1);
            message->type = Message::Type::WindowRenameRequest;
            message->data_len = sizeof(WindowRenameRequest) + name.size() + 1;
            auto& request = message->data.window_rename_request;
            request.wid = wid;
            strcpy(request.name, name.string());
            return SharedPtr<Message>(message);
        }

        wid_t wid;
        char name[0];
    };

    size_t total_size() const { return sizeof(Message) + data_len; };

    Type type { Type::Invalid };
    int data_len { 0 };
    union {
        CreateWindowRequest create_window_request;
        CreateWindowResponse create_window_response;
        RemoveWindowRequest remove_window_request;
        RemoveWindowResponse remove_window_response;
        ChangeWindowVisibilityRequeset change_window_visibility_request;
        ChangeWindowVisibilityResponse change_window_visibility_response;
        SwapBufferRequest swap_buffer_request;
        KeyEventMessage key_event_message;
        MouseEventMessage mouse_event_message;
        WindowDidResizeMessage window_did_resize_message;
        WindowReadyToResizeMessage window_ready_to_resize_message;
        WindowReadyToResizeResponse window_ready_to_resize_response;
        WindowClosedEventMessage window_closed_event_messasge;
        WindowRenameRequest window_rename_request;
    } data;
};

};

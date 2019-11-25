#pragma once

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t wid_t;

namespace WindowServer {

struct Message {
    enum class Type {
        Invalid,
        CreateWindowRequest,
        CreateWindowResponse,
        RemoveWindowRequest,
        RemoveWindowResponse
    };

    struct CreateWindowRequest {
        CreateWindowRequest(int xx, int yy, int wwidth, int hheight)
            : x(xx), y(yy), width(wwidth), height(hheight)
        {
        }

        static std::shared_ptr<Message> create(int x, int y, int width, int height)
        {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(CreateWindowRequest));
            message->type = Message::Type::CreateWindowRequest;
            message->data_len = sizeof(CreateWindowRequest);
            new (&message->data.create_window_request) CreateWindowRequest(x, y, width, height);
            return std::shared_ptr<Message>(message);
        }

        int x;
        int y;
        int width;
        int height;
    };

    struct CreateWindowResponse {
        static std::shared_ptr<Message> create(wid_t id, size_t size, const char* path)
        {
            size_t path_len = strlen(path);
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(CreateWindowResponse) + path_len + 1);
            message->type = Message::Type::CreateWindowResponse;
            message->data_len = sizeof(CreateWindowResponse) + path_len + 1;
            auto& response = message->data.create_window_response;
            response.window_id = id;
            response.shared_buffer_size = size;
            strcpy(response.shared_buffer_path, path);
            return std::shared_ptr<Message>(message);
        }

        wid_t window_id;
        size_t shared_buffer_size;
        char shared_buffer_path[0];
    };

    struct RemoveWindowRequest {
        static std::shared_ptr<Message> create(wid_t id)
        {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(RemoveWindowRequest));
            message->type = Message::Type::RemoveWindowRequest;
            message->data_len = sizeof(RemoveWindowRequest);
            auto& request = message->data.remove_window_request;
            request.wid = id;
            return std::shared_ptr<Message>(message);
        }

        wid_t wid;
    };

    struct RemoveWindowResponse {
        static std::shared_ptr<Message> create(bool success)
        {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(RemoveWindowResponse));
            message->type = Message::Type::RemoveWindowResponse;
            message->data_len = sizeof(RemoveWindowResponse);
            auto& response = message->data.remove_window_response;
            response.success = success;
            return std::shared_ptr<Message>(message);
        }

        bool success;
    };

    size_t total_size() const { return sizeof(Message) + data_len; };

    Type type { Type::Invalid };
    int data_len { 0 };
    union {
        CreateWindowRequest create_window_request;
        CreateWindowResponse create_window_response;
        RemoveWindowRequest remove_window_request;
        RemoveWindowResponse remove_window_response;
    } data;
};

};
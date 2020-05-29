#pragma once

#include <liim/pointers.h>
#include <liim/string.h>
#include <stdlib.h>

namespace ClipboardServer {

struct Message {
    enum class Type {
        Invalid,
        GetContentsRequest,
        SetContentsRequest,
        GetContentsResponse,
        SetContentsResponse,
    };

    struct GetContentsRequest {
        static UniquePtr<Message> create(const String& type) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(GetContentsRequest) + type.size() + 1);
            message->type = Message::Type::GetContentsRequest;
            message->data_len = sizeof(GetContentsRequest) + type.size() + 1;
            auto& request = message->data.get_contents_request;
            strcpy(request.content_type, type.string());
            return UniquePtr<Message>(message);
        }

        char content_type[0];
    };

    struct SetContentsRequest {
        static UniquePtr<Message> create(const String& type, const char* data, size_t data_len) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(SetContentsRequest) + type.size() + 1 + data_len);
            message->type = Message::Type::SetContentsRequest;
            message->data_len = sizeof(SetContentsRequest) + type.size() + 1 + data_len;
            auto& request = message->data.set_contents_request;
            request.type_length = type.size() + 1;
            request.data_length = data_len;
            strcpy(request.payload, type.string());
            memcpy(request.payload + type.size() + 1, data, data_len);
            return UniquePtr<Message>(message);
        }

        char* content_type() { return payload; }
        char* data() { return payload + data_length; }

        size_t type_length;
        size_t data_length;
        char payload[0];
    };

    struct GetConentsResponse {
        static UniquePtr<Message> create(const char* data, size_t data_len) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(GetConentsResponse) + data_len);
            message->type = Message::Type::GetContentsResponse;
            message->data_len = sizeof(GetContentsRequest) + data_len;
            auto& response = message->data.get_contents_response;
            response.success = !!data;
            memcpy(response.data, data, data_len);
            return UniquePtr<Message>(message);
        }

        bool success;
        char data[0];
    };

    struct SetContentsResponse {
        static UniquePtr<Message> create(bool success) {
            auto* message = (Message*) malloc(sizeof(Message) + sizeof(SetContentsResponse));
            message->type = Message::Type::SetContentsResponse;
            message->data_len = sizeof(SetContentsResponse);
            auto& response = message->data.set_contents_response;
            response.success = success;
            return UniquePtr<Message>(message);
        }

        bool success;
    };

    size_t total_size() const { return sizeof(Message) + data_len; };

    Type type { Type::Invalid };
    int data_len { 0 };
    union {
        GetContentsRequest get_contents_request;
        SetContentsRequest set_contents_request;
        GetConentsResponse get_contents_response;
        SetContentsResponse set_contents_response;
    } data;
};

};

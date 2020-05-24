#pragma once

#include "shmem_queue.hpp"

#include <string>
#include <memory>

class OneWayChannel;

using OneWayChannelRef = std::shared_ptr<OneWayChannel>;

class OneWayChannel {
public:
    OneWayChannel() {
        handle_.Initialize();
    }

    ~OneWayChannel() {
        // handle_.Deinitialize();
    }

    static OneWayChannelRef New() {
        return std::make_shared<OneWayChannel>();
    }

    void Write(std::string_view data) const {
        shmem_queue::WriteMessage(handle_, data);
    }

    void Read(std::string* out) const {
        shmem_queue::ReadMessage(handle_, out);
    }

private:
    struct alignas(128) Block {
        static constexpr int64_t size = 1024 * 1024; // ~1mb buffer
        char data[size];
    };

    std::unique_ptr<Block> block_{new Block{}};
    shmem_queue::Handle handle_{{block_->data, block_->size}};
};


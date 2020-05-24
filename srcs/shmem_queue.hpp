#pragma once

#include <string>

struct MemView {
    char* begin = nullptr;
    int64_t size = 0;

    MemView& operator+=(int64_t shift) {
        begin += shift;
        size -= shift;
        return *this;
    }

    MemView operator+(int64_t shift) const {
        MemView cpy{*this};
        cpy += shift;
        return cpy;
    }

    explicit operator bool() const {
        return size != 0;
    }
};

namespace shmem_queue {
// handle to an SPSC queue
class Handle {
public:
    explicit Handle(MemView mem) noexcept : mem_{mem} {
    }

    void Initialize() const;
    void Deinitialize() const;

    MemView Mem() const {
        return mem_;
    }

private:
    MemView mem_;
};

// aka queue.Pop
// may block in the middle of a call, without fully reading message
void ReadMessage(Handle handle, std::string* out);

// aka queue.Push
// may block in the middle of a call, without fully submitting message
void WriteMessage(Handle handle, std::string_view in);
}  // namespace shmem_queue

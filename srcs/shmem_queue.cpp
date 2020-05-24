#include "shmem_queue.hpp"

#include <atomic>
#include <cassert>
#include <mutex>
#include <type_traits>
#include <condition_variable>
#include <cstring>

namespace shmem_queue {
namespace {
struct ShmemBlockHeaderInit {
    enum class HeaderState : char {
        NotInitialized = 0,
        Initializing = 1,
        Initialized = 2,
    };

    std::atomic<HeaderState> state{HeaderState::Initialized};
};

// essentially a state of SPSC queue
struct ShmemBlockHeader : ShmemBlockHeaderInit {
    static constexpr int64_t kMagicNumber = 123234;

    int64_t magic_number{kMagicNumber};  // just some random number to check that we are indeed
                                         // looking at an initialized header
    std::mutex mut;

    std::condition_variable chunk_ready_cv;  // notifications about chunk status
    bool chunk_ready = false;
    MemView data;
};

struct DataChunkHeader {
    bool is_last;
    char size[8];
};
static_assert(alignof(DataChunkHeader) == 1);
static_assert(std::is_trivially_destructible_v<DataChunkHeader>);
static_assert(std::is_trivially_constructible_v<DataChunkHeader>);
static_assert(sizeof(int64_t) == 8);

struct DataChunkHeaderView {
    bool is_last = false;
    int64_t size = 0;
};

struct DataChunkInfo {
    DataChunkHeader* header = nullptr;
    MemView data;
};

DataChunkInfo ReadInfo(const MemView chunk) {
    auto* header = reinterpret_cast<DataChunkHeader*>(chunk.begin);
    int64_t size = 0;
    std::memcpy(&size, header->size, sizeof(int64_t));
    return {header, {chunk.begin + sizeof(DataChunkHeader), size}};
}

void WriteHeader(DataChunkHeaderView new_header, MemView* chunk) {
    auto* header = reinterpret_cast<DataChunkHeader*>(chunk->begin);
    std::memcpy(&header->is_last, &new_header.is_last, sizeof(bool));
    std::memcpy(&header->size, &new_header.size, sizeof(int64_t));
    *chunk += sizeof(DataChunkHeader);
}

void AssertCanHoldBlock(MemView block) {
    (void)block;
    assert((size_t)block.begin % alignof(ShmemBlockHeader) == 0);
    assert(block.size >= static_cast<int64_t>(sizeof(ShmemBlockHeader) + sizeof(DataChunkHeader) + 1));
}

void AssertHoldsBlock(MemView block) {
    AssertCanHoldBlock(block);
    auto* header = reinterpret_cast<ShmemBlockHeader*>(block.begin);
    (void)header;
    assert(header->magic_number == ShmemBlockHeader::kMagicNumber);
}

ShmemBlockHeader* GetBlockHeader(MemView block) {
    AssertHoldsBlock(block);
    return reinterpret_cast<ShmemBlockHeader*>(block.begin);
}

void InitMemRegion(MemView mem) {
    AssertCanHoldBlock(mem);

    auto waitInitialized = [](const ShmemBlockHeaderInit* header) {
        while (header->state.load(std::memory_order_acquire) !=
               ShmemBlockHeader::HeaderState::Initialized) {
        }
    };

    auto* init = reinterpret_cast<ShmemBlockHeaderInit*>(mem.begin);
    if (auto expected = ShmemBlockHeader::HeaderState::NotInitialized;
        !init->state.compare_exchange_strong(expected, ShmemBlockHeader::HeaderState::Initializing,
                                             std::memory_order_relaxed)) {
        waitInitialized(init);
        return;
    }

    auto* header = new (reinterpret_cast<ShmemBlockHeader*>(init)) ShmemBlockHeader{};
    header->data.begin = mem.begin + sizeof(ShmemBlockHeader);
    header->data.size = (mem.begin + mem.size) - header->data.begin;
    header->state.store(ShmemBlockHeader::HeaderState::Initialized,
                              std::memory_order_release);
}

void DeinitMemRegion(MemView mem) {
    AssertHoldsBlock(mem);

    auto* header = reinterpret_cast<ShmemBlockHeader*>(mem.begin);
    if (header->state.load(std::memory_order_acquire) != ShmemBlockHeader::HeaderState::Initialized) {
        assert(false);
    }
    header->magic_number = -1;
    header->~ShmemBlockHeader();
}

// push back new bytes to out
// return value : is the chunk final
bool ReadChunk(ShmemBlockHeader* block, std::string* out) {
    std::unique_lock lock{block->mut};
    block->chunk_ready_cv.wait(lock, [=] { return block->chunk_ready; });

    block->chunk_ready = false;
    const DataChunkInfo info = ReadInfo(block->data);

    const auto out_init_size = out->size();
    out->resize(out_init_size + info.data.size);
    std::memcpy(out->data() + out_init_size, info.data.begin, info.data.size);

    block->chunk_ready_cv.notify_one();
    return !info.header->is_last;
}

void ReadMessage(ShmemBlockHeader* block, std::string* out) {
    while (ReadChunk(block, out)) {
    }
}

void WriteChunk(ShmemBlockHeader* block, MemView* data) {
    std::unique_lock lock{block->mut};
    block->chunk_ready_cv.wait(lock, [=] { return !block->chunk_ready; });

    block->chunk_ready = true;
    auto chunk_size = std::min(data->size, (int64_t)(block->data.size - sizeof(DataChunkHeader)));

    MemView out = block->data;

    WriteHeader({chunk_size == data->size, chunk_size}, &out);
    std::memcpy(out.begin, data->begin, chunk_size);
    *data += chunk_size;

    block->chunk_ready_cv.notify_one();
}

void WriteMessage(ShmemBlockHeader* block, MemView message) {
    while (message) {
        WriteChunk(block, &message);
    }
}

}  // namespace

void Handle::Initialize() const {
    InitMemRegion(mem_);
}

void Handle::Deinitialize() const {
    DeinitMemRegion(mem_);
}

void ReadMessage(Handle handle, std::string* out) {
    ShmemBlockHeader* header = GetBlockHeader(handle.Mem());
    ReadMessage(header, out);
}

void WriteMessage(Handle handle, std::string_view in) {
    ShmemBlockHeader* header = GetBlockHeader(handle.Mem());
    MemView in_view{const_cast<char*>(in.data()), static_cast<int64_t>(in.size())};
    WriteMessage(header, in_view);
}
}  // namespace shmem_queue

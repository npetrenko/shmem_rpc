#include "shmem_queue.hpp"

#include <gtest/gtest.h>

#include <thread>
#include <random>
#include <cassert>

template <size_t tsize = 1024>
struct alignas(128) Block  {
    char data[tsize];
    static constexpr int64_t size = tsize;
};

template <size_t size = 1024>
std::unique_ptr<Block<size>> NewBlock() {
    return std::make_unique<Block<size>>();
}


std::string RandomString(size_t lenght, std::mt19937* rd) {
    std::uniform_int_distribution<char> distr;
    std::string out;
    out.reserve(lenght);
    for (size_t i = 0; i < lenght; ++i) {
        out.push_back(distr(*rd));
    }

    return out;
}

class ShmemQueue : public ::testing::Test {
public:
    void SetUp() override {
        handle.Initialize();
    }

    void TearDown() override {
        handle.Deinitialize();
        if (new_thread) {
            new_thread->join();
        }
    }

    template <class Func>
    void InNewThread(Func func) {
        assert(!new_thread);
        new_thread = std::make_unique<std::thread>(func);
    }

    std::unique_ptr<Block<>> block = NewBlock();
    shmem_queue::Handle handle{{block->data, block->size}};

    std::unique_ptr<std::thread> new_thread;
};

TEST_F(ShmemQueue, HelloWorld) {
    std::string out;
    std::string_view in{"Hello, world!"};

    InNewThread([&] {
        WriteMessage(handle, in);
    });
    ReadMessage(handle, &out);

    ASSERT_EQ(in, out);
}

TEST_F(ShmemQueue, LongInput) {
    std::mt19937 rd{1234};
    std::string out;
    const std::string in = RandomString((2048 + 313)*43 + 31, &rd);

    InNewThread([&] {
        WriteMessage(handle, in);
    });

    ReadMessage(handle, &out);

    ASSERT_EQ(in, out);
}

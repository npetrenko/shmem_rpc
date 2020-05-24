add_library(shmem_queue shmem_queue.cpp)

add_grpc_library(test_grpc rpc.proto)

add_benchmark(bench bench.cpp)
target_link_libraries(bench test_grpc shmem_queue)

add_executable(tests test.cpp)
add_test(tests tests)
target_link_libraries(tests shmem_queue gtest gtest_main)

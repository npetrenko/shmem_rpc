project(shmem_rpc)

cmake_minimum_required(VERSION 3.8)

set(WFLAGS "-Werror -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wenum-compare -Wnon-virtual-dtor -Woverloaded-virtual -Wredundant-decls -Wsign-promo")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG -g -O3 -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_ASAN "${CMAKE_CXX_FLAGS} -g -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_TSAN "${CMAKE_CXX_FLAGS} -g -fsanitize=thread")

include(cmake/FindGrpc.cmake)
include(cmake/AddGrpc.cmake)
include(cmake/AddBench.cmake)

find_package(GTest REQUIRED)
enable_testing()

find_package(Protobuf REQUIRED)

find_package(gRPC CONFIG REQUIRED)

include_directories(.)
link_libraries(pthread)

add_subdirectory(srcs)

function(add_grpc_library target_name)
  list(REMOVE_AT ARGV 0)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${ARGV})
  protobuf_generate_grpc_cpp(GRPC_SRCS GRPC_HDRS ${ARGV})

  add_library(${target_name} ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})

  target_link_libraries(${target_name} gRPC::grpc++_reflection protobuf::libprotobuf)
  target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

add_subdirectory(proto)
add_library(rpc-protos OBJECT 
    "proto/rpc.proto"
)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto")
target_include_directories(rpc-protos PUBLIC "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>")

protobuf_generate(
  TARGET rpc-protos
  IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/proto"
  PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

target_link_libraries(rpc-protos PUBLIC protobuf::libprotobuf)

add_library(rpc_message rpc_message.h rpc_message.cpp)

target_include_directories(rpc_message PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_library(rpc_server rpc_server.h rpc_server.cpp)

target_include_directories(rpc_server PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(rpc_server rpc_message rpc-protos)
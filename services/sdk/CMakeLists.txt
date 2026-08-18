add_subdirectory(proto)
add_library(sdk-protos OBJECT 
    "proto/tmf.proto"
)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto")
target_include_directories(sdk-protos PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

protobuf_generate(
  TARGET sdk-protos
  IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/proto"
  PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)

target_link_libraries(sdk-protos PUBLIC protobuf::libprotobuf)

add_subdirectory(tmf)

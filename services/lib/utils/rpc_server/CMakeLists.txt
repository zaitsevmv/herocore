add_library(rpc_message rpc_message.h rpc_message.cpp)

target_include_directories(rpc_message PRIVATE
    ${CMAKE_SOURCE_DIR}
)

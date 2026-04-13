add_library(tcp_awaiters tcp_awaiters.h tcp_awaiters.cpp)

target_include_directories(tcp_awaiters PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

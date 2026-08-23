add_library(tcp_awaiters tcp_awaiters.h tcp_awaiters.cpp)

target_include_directories(tcp_awaiters PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_library(tcp_requests tcp_requests.h tcp_requests.cpp)

target_include_directories(tcp_requests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(tcp_requests tcp_awaiters reactor)

add_library(http_message http_message.h http_message.cpp)

target_include_directories(http_message PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_library(http_server http_server.h http_server.cpp)

target_include_directories(http_server PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(http_server tcp_awaiters tcp_requests http_message reactor thread_pool)

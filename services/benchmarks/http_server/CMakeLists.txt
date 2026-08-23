add_executable(http_server_bench main.cpp)

target_include_directories(http_server_bench PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(http_server_bench tcp_awaiters tcp_requests http_message http_server reactor thread_pool)

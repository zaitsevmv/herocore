add_executable(files_bench main.cpp)

target_include_directories(files_bench PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(files_bench reactor files_awaiters thread_pool)

add_executable(timed_run_bench main.cpp)

target_include_directories(timed_run_bench PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(timed_run_bench timed_runner thread_pool)

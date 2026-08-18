add_library(thread_pool thread_pool.h thread_pool.cpp)

target_include_directories(thread_pool PRIVATE
    ${CMAKE_SOURCE_DIR}
)

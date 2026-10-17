add_library(executor executor.h thread_pool.h thread_pool.cpp)

target_include_directories(executor PRIVATE
    ${CMAKE_SOURCE_DIR}
)

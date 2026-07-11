add_library(files_awaiters files_awaiters.h files_awaiters.cpp)

target_include_directories(files_awaiters PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

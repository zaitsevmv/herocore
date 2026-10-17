add_library(files_awaiters files_awaiters.h files_awaiters.cpp)

target_link_libraries(files_awaiters PUBLIC reactor)

target_include_directories(files_awaiters PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_library(files_operations files_operations.h files_operations.cpp)

target_link_libraries(files_operations PUBLIC files_awaiters reactor)

target_include_directories(files_operations PRIVATE
    ${CMAKE_SOURCE_DIR}
)

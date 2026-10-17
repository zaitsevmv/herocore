add_library(reactor reactor.h reactor.cpp)

target_link_libraries(reactor uring)

target_include_directories(reactor PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_library(timed_runner timed_runner.h timed_runner.cpp)

target_include_directories(timed_runner PRIVATE
    ${CMAKE_SOURCE_DIR}
)

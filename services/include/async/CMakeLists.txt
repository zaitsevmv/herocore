add_library(reactor reactor.h reactor.cpp)

target_link_libraries(reactor uring)

target_include_directories(reactor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

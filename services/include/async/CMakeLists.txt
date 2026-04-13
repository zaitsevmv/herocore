add_library(reactor reactor.h reactor.cpp)

target_include_directories(reactor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

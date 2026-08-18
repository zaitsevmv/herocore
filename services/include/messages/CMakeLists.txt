add_library(messages messages.h messages.cpp)

target_include_directories(messages PRIVATE
    ${CMAKE_SOURCE_DIR}
)

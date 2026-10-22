add_library(tmf tmf.h tmf.cpp)

target_include_directories(tmf PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(tmf sdk-protos)

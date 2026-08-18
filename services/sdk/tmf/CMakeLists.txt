add_library(tmf tmf.h tmf.cpp)
message(${CMAKE_BUILD_WITH_INSTALL_NAME_DIR}/sdk/proto)

target_include_directories(tmf PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(tmf sdk-protos)

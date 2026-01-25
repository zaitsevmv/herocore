add_library(async_task INTERFACE)

target_include_directories(async_task INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(tests)

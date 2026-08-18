cmake_minimum_required(VERSION 3.20)

project(herocore_services CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED liburing)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/install/protobuf")
find_package(Protobuf CONFIG REQUIRED)

add_subdirectory(include)
add_subdirectory(sdk)
add_subdirectory(benchmarks)

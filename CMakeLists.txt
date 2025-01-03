cmake_minimum_required(VERSION 3.10)
project(gplay)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(externals/SDL EXCLUDE_FROM_ALL)
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
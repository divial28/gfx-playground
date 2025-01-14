cmake_minimum_required(VERSION 3.10)
project(gplay)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDL
set(SDL_AUDIO_DEFAULT OFF)
set(SDL_GPU_DEFAULT OFF)
set(SDL_RENDER_DEFAULT OFF)
set(SDL_CAMERA_DEFAULT OFF)
set(SDL_JOYSTICK_DEFAULT OFF)
set(SDL_HAPTIC_DEFAULT OFF)
set(SDL_HIDAPI_DEFAULT OFF)
set(SDL_POWER_DEFAULT OFF)
set(SDL_SENSOR_DEFAULT OFF)
set(SDL_DIALOG_DEFAULT OFF)
set(SDL_THREADS_DEFAULT OFF)
add_subdirectory(externals/SDL EXCLUDE_FROM_ALL)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable( ${PROJECT_NAME} 
    ${SRC_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/glad/src/glad.c
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_widgets.cpp
)

target_include_directories( ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/glm
)

target_link_libraries( ${PROJECT_NAME}
    PRIVATE SDL3::SDL3
)

target_compile_definitions( ${PROJECT_NAME} 
    PRIVATE "OPENGL_DEBUG"
)


add_executable( frame_rate_test 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/frame_rate_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/glad/src/glad.c
)

target_include_directories( frame_rate_test 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/externals/glad/include
)

target_link_libraries( frame_rate_test 
    PRIVATE SDL3::SDL3
)

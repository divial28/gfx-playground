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

add_executable( ${PROJECT_NAME} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/glad/src/glad.c
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/backends/imgui_impl_sdl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories( ${PROJECT_NAME} PRIVATE
    externals/glad/include
    externals/imgui
)

target_link_libraries( ${PROJECT_NAME}
    PRIVATE SDL3::SDL3
)
cmake_minimum_required(VERSION 3.28)
project(c_overlay C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC_FILES src/*.c)
file(GLOB_RECURSE OVERLAY_FILES src/overlay/*.c)
file(GLOB_RECURSE CIMGUI include/cimgui/*.cpp)

add_executable(c_overlay
        #src/main.c
        ${SRC_FILES}
        ${OVERLAY_FILES}
        ${CIMGUI}
)

# Include headers
target_include_directories(c_overlay PRIVATE include/cimgui)
target_include_directories(c_overlay PRIVATE src)
target_include_directories(c_overlay PRIVATE src/overlay)

# Include DirectX11
target_link_libraries(c_overlay d3d11 dxgi d3dcompiler dwmapi)
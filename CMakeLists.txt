cmake_minimum_required(VERSION 3.20)
project(gviz CXX C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# gviz (and everything it statically links, e.g. `planar` below) builds as a
# shared library, so every object file needs position-independent code even
# on platforms where that isn't the default for static archives.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(gviz ${GVIZ_LIB_TYPE}
	# cpp files here
	src/ecs/index_space.cpp

	src/graph/graph.cpp
	src/graph/subgraph.cpp

	src/ds/bfs_scratch.cpp

	src/layout/placement/helpers.cpp
)

target_include_directories(gviz
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

set_target_properties(gviz PROPERTIES
    VERSION 0.1.0
    SOVERSION 0
    # MSVC/MinGW shared libs need explicit exports; auto-export every symbol
    # rather than hand-annotating ~20 public headers with __declspec. No
    # effect on non-Windows targets.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(gviz
    PRIVATE
    m
)

add_executable(main main.cpp)
set_target_properties(main PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)
target_link_libraries(main PRIVATE gviz)

option(GVIZ_BUILD_RENDER "Build the WebGPU/GLFW graph renderer + demo" OFF)

if(GVIZ_BUILD_RENDER)
    if(APPLE)
        enable_language(OBJC)
        enable_language(OBJCXX)
    endif()

    set(GVIZ_WGPU_NATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/wgpu-native" CACHE PATH
        "Path to extracted wgpu-native release (must contain include/ and lib/)")
    if(NOT EXISTS "${GVIZ_WGPU_NATIVE_DIR}/include/webgpu/webgpu.h")
        message(FATAL_ERROR
            "wgpu-native not found at ${GVIZ_WGPU_NATIVE_DIR}.\n"
            "Run ./scripts/setup-deps.sh once, or set -DGVIZ_WGPU_NATIVE_DIR=...")
    endif()

    add_library(wgpu_native STATIC IMPORTED)
    if(WIN32)
        set_target_properties(wgpu_native PROPERTIES
            IMPORTED_LOCATION "${GVIZ_WGPU_NATIVE_DIR}/lib/wgpu_native.lib")
    else()
        set_target_properties(wgpu_native PROPERTIES
            IMPORTED_LOCATION "${GVIZ_WGPU_NATIVE_DIR}/lib/libwgpu_native.a")
    endif()
    target_include_directories(wgpu_native INTERFACE "${GVIZ_WGPU_NATIVE_DIR}/include")

    if(APPLE)
        target_link_libraries(wgpu_native INTERFACE
            "-framework CoreFoundation"
            "-framework QuartzCore"
            "-framework Metal"
        )
    elseif(WIN32)
        target_link_libraries(wgpu_native INTERFACE
            d3dcompiler ws2_32 userenv bcrypt ntdll opengl32 Propsys RuntimeObject)
    else()
        target_link_libraries(wgpu_native INTERFACE m dl)
    endif()

    set(GVIZ_GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/glfw" CACHE PATH
        "Path to GLFW source tree")
    if(NOT EXISTS "${GVIZ_GLFW_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "GLFW not found at ${GVIZ_GLFW_DIR}.\n"
            "Run ./scripts/setup-deps.sh once, or set -DGVIZ_GLFW_DIR=...")
    endif()

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${GVIZ_GLFW_DIR}" glfw-build EXCLUDE_FROM_ALL)

    add_library(gvizRender STATIC
        src/render/renderer.cpp
    )
    if(APPLE)
        target_sources(gvizRender PRIVATE src/render/surface_cocoa.mm)
        target_link_libraries(gvizRender PRIVATE "-framework AppKit")
    endif()

    target_include_directories(gvizRender
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/render
    )
    target_link_libraries(gvizRender
        PUBLIC gviz wgpu_native
        PRIVATE glfw
    )
    if(NOT MSVC)
        target_compile_options(gvizRender PRIVATE -Wall -Wextra)
    endif()

    add_executable(renderDemo examples/renderDemo.cpp)
    target_link_libraries(renderDemo PRIVATE gvizRender gviz)
endif()

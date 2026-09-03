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
	src/ecs/admin.cpp
	src/graph/graph.cpp
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

add_library(catch2 STATIC ${CMAKE_CURRENT_SOURCE_DIR}/../third-party/catch2/catch_amalgamated.cpp)
target_include_directories(catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../third-party/catch2)

file(GLOB_RECURSE GVIZ_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(gviz_tests ${GVIZ_TEST_SOURCES})

target_include_directories(gviz_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(gviz_tests PRIVATE gviz catch2)

include(CTest)
add_test(NAME gviz_tests COMMAND gviz_tests)

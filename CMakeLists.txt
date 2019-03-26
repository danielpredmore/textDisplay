cmake_minimum_required(VERSION 3.10.2)

project (myDisplay)

include_directories(lib)

file(GLOB LIB lib/*.c)
file(GLOB TESTS tests/*.c)

add_library (display ${LIB})

add_executable(displayTest ${TESTS})
target_link_libraries (displayTest LINK_PUBLIC display)
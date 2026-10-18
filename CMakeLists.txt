cmake_minimum_required(VERSION 3.10)
project(wlang VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

file(GLOB_RECURSE WLANG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(wl ${WLANG_SOURCES})
target_compile_options(wl PRIVATE -fsanitize=address -g)
target_link_options(wl PRIVATE -fsanitize=address)
target_include_directories(wl
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(wl PRIVATE m)

cmake_minimum_required(VERSION 3.10)
project(wlang VERSION 0.1.0 LANGUAGES C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)
set(RT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/runtime/rtw.s")
set(RT_OBJ "${CMAKE_CURRENT_BINARY_DIR}/rtw.o")

add_custom_command(
  OUTPUT ${RT_OBJ}
  COMMAND ${CMAKE_ASM_COMPILER} ${RT_SRC} -c -o ${RT_OBJ}
  DEPENDS ${RT_SRC}
  COMMENT "compiling runtime..."
)

add_custom_target(wlang_runtime ALL DEPENDS ${RT_OBJ})

file(GLOB_RECURSE WLANG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(wl ${WLANG_SOURCES})
add_dependencies(wl wlang_runtime)

target_compile_definitions(wl PRIVATE
  WCOMPILER_VERSION_STR="${PROJECT_VERSION}"
)

target_compile_options(wl PRIVATE -fsanitize=address -g)
target_link_options(wl PRIVATE -fsanitize=address)

target_include_directories(wl
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(wl PRIVATE m)

install(FILES ${RT_OBJ} DESTINATION lib/wlang)
install(TARGETS wl DESTINATION bin)

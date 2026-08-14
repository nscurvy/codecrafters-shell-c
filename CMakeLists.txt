cmake_minimum_required(VERSION 3.13)

project(codecrafters-shell)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(shell ${SOURCE_FILES}
        src/builtins.h
        src/builtin.c
        src/parser.h
        src/parser.c
        src/exec.c
        src/exec.h
        src/expand.c
        src/expand.h)

target_link_libraries(shell PRIVATE readline)

cmake_minimum_required(VERSION 3.13)

project(codecrafters-shell)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)


set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(shell ${SOURCE_FILES}
        src/builtins.h
        src/builtin.c
        src/parser.h
        src/parser.c
        src/exec.c
        src/exec.h
        src/expand.c
        src/expand.h
        src/completion.c
        src/completion.h
        src/nullability.h)

target_link_libraries(shell PRIVATE readline PkgConfig::GLIB)

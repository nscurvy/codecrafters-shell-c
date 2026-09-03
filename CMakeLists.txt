cmake_minimum_required(VERSION 3.13)

project(codecrafters-shell)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)
file(GLOB_RECURSE LIB_FILES src/*.c src/*.h)
if ($ENV{HOME_MACHINE})
file(GLOB_RECURSE TEST_FILES tests/*.c)
endif()

list(REMOVE_ITEM LIB_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)

enable_testing()
if ($ENV{HOME_MACHINE})
find_package(PkgConfig REQUIRED)
    pkg_check_modules(CHECK REQUIRED check)
endif()
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)


set(CMAKE_C_STANDARD 23) # Enable the C23 standard

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O1 -D_FORTIFY_SOURCE=3)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer -g -O1 -D_FORTIFY_SOURCE=3)
    add_link_options(-fsanitize=undefined)
endif()

add_compile_options(-Wall -Wextra -Wconversion -Wsign-conversion -Wcast-align -Wshadow -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wformat-security -Wstrict-overflow=5 -Wundef -Wuninitialized -Wswitch-enum)

add_executable(shell ${SOURCE_FILES}
        src/builtins.h
        src/builtins.c
        src/declare.c
        src/declare.h
        src/parser.h
        src/parser.c
        src/exec.c
        src/exec.h
        src/expand.c
        src/expand.h
        src/completion.c
        src/completion.h
        src/nullability.h
        src/jobs.h
        src/jobs.c
        src/common.h
        src/wordlist.c
        src/wordlist.h)

add_library(shell_lib ${LIB_FILES}
        src/wordlist.c
        src/wordlist.h)
set_property(TARGET shell_lib PROPERTY OUTPUT_NAME shell)

if($ENV{HOME_MACHINE})
add_executable(shell_test ${TEST_FILES}
        src/wordlist.c
        src/wordlist.h)
target_include_directories(shell_test PRIVATE src
                                      PRIVATE ${CHECK_INCLUDE_DIRS})

target_link_libraries(shell_test PRIVATE shell_lib ${CHECK_LIBRARIES} readline)
add_test(NAME ShellUnitTests COMMAND shell_test)
endif()
target_link_libraries(shell PRIVATE readline)
target_link_libraries(shell_lib PRIVATE readline)


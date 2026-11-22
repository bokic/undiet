cmake_minimum_required(VERSION 3.5)

SET(CMAKE_SKIP_RPATH true)

set(CMAKE_VERBOSE_MAKEFILE ON FORCE)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX /usr)

project(undiet LANGUAGES C)

add_library(libundiet SHARED
    undiet.c
    undiet.h
)
set_target_properties(libundiet PROPERTIES OUTPUT_NAME "undiet")
set_target_properties(libundiet PROPERTIES COMPILE_FLAGS "-save-temps")

add_executable(undiet
    main.c
    undiet.h
)
set_target_properties(undiet PROPERTIES COMPILE_FLAGS "-save-temps")
target_link_libraries(undiet libundiet)

install(TARGETS
    libundiet
    undiet
)

enable_testing()
add_executable(test_undiet tests/test_undiet.c)
target_include_directories(test_undiet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_undiet libundiet)
add_test(NAME test_undiet COMMAND test_undiet)
set_tests_properties(test_undiet PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:libundiet>")

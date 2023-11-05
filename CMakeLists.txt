cmake_minimum_required(VERSION 3.5)

SET(CMAKE_SKIP_RPATH true)

project(undiet LANGUAGES C)
add_library(libundiet SHARED
    undiet.c
    undiet.h
)
set_target_properties(libundiet PROPERTIES OUTPUT_NAME "undiet")

add_executable(undiet
    main.c

    undiet.h
)
target_link_libraries(undiet libundiet)

set(CMAKE_INSTALL_PREFIX /usr)
install(TARGETS
    libundiet
    undiet
)

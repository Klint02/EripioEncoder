cmake_minimum_required(VERSION 3.28)
project(eripio_encoder)

set(SOURCE_FILES
    src/main.cpp
    src/encodeLib.cpp)

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(eripio_encoder raylib)
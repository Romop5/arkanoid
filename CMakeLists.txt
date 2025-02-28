cmake_minimum_required(VERSION 3.27)

project(arkanoid)

find_package(SDL2 REQUIRED)

add_executable(arkanoid WIN32
    src/main.cpp
)
target_link_libraries(arkanoid PRIVATE SDL2::SDL2 SDL2::SDL2main)
set_property(TARGET arkanoid PROPERTY CXX_STANDARD 20)
cmake_minimum_required(VERSION 3.27)

project(arkanoid)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

file(GLOB game_sources "src/game/*")
add_library(game)
target_sources(game PRIVATE ${game_sources})
target_link_libraries(game PUBLIC SDL2::SDL2 SDL2_image::SDL2_image sdl_ttf::sdl_ttf)
target_include_directories(game PUBLIC "src/")
set_property(TARGET game PROPERTY CXX_STANDARD 20)

file(GLOB akranoid_sources "src/main.cpp")
add_executable(arkanoid WIN32)
target_sources(arkanoid PRIVATE ${akranoid_sources})
target_link_libraries(arkanoid PRIVATE game SDL2::SDL2main)
set_property(TARGET arkanoid PROPERTY CXX_STANDARD 20)

add_custom_command(TARGET arkanoid POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
)

file(GLOB sandbox_sources "src/sandbox.cpp")
add_executable(sandbox)
target_sources(sandbox PRIVATE ${sandbox_sources})
target_link_libraries(sandbox PRIVATE game SDL2::SDL2main)
set_property(TARGET sandbox PROPERTY CXX_STANDARD 20)
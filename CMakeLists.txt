cmake_minimum_required(VERSION 3.25)
project(StarFleetBattles VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

add_executable(sfb WIN32
    src/main.cpp
    src/hex.cpp
    src/board.cpp
    src/ship.cpp
    src/renderer_sdl.cpp
)

target_include_directories(sfb PRIVATE include)
target_link_libraries(sfb PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
)

if(MSVC)
    target_compile_options(sfb PRIVATE /W4)
endif()

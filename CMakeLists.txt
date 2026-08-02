cmake_minimum_required(VERSION 3.21)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

add_executable(chip8
	src/main.cpp
	src/chip8.cpp
	src/platform.cpp
)

if (MSVC)
	target_compile_options(chip8 PRIVATE /W4)
else()
	target_compile_options(chip8 PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(chip8 PRIVATE
	$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
	$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
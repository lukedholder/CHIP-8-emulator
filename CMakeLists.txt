cmake_minimum_required(VERSION 3.21)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

# Emulator core - deliberately has no SDL dependency
add_library(chip8core STATIC src/chip8.cpp)
target_include_directories(chip8core PUBLIC src)

add_executable(chip8 src/main.cpp src/platform.cpp)
target_link_libraries(chip8 PRIVATE
	chip8core
	$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
	$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

if (MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

enable_testing()
find_package(Catch2 3 CONFIG REQUIRED)

add_executable(chip8_tests tests/test_chip8.cpp)
target_link_libraries(chip8_tests PRIVATE chip8core Catch2::Catch2WithMain)

add_test(NAME chip8_tests COMMAND chip8_tests)

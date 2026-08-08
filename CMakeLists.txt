cmake_minimum_required(VERSION 3.21)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warning flags, defined once and applied explicitly to each target below
if (MSVC)
	set(CHIP8_WARNINGS /W4)
else()
	set(CHIP8_WARNINGS -Wall -Wextra -Wpedantic)
endif()

find_package(SDL2 CONFIG REQUIRED)

# Emulator core - deliberately has no SDL dependency
add_library(chip8core STATIC 
	src/chip8.cpp
	src/disasm.cpp
)
target_include_directories(chip8core PUBLIC src)
target_compile_options(chip8core PRIVATE ${CHIP8_WARNINGS})

add_executable(chip8
	src/main.cpp
	src/platform.cpp
)
target_compile_options(chip8 PRIVATE ${CHIP8_WARNINGS})
target_link_libraries(chip8 PRIVATE
	chip8core
	$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
	$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

enable_testing()

find_package(Catch2 3 CONFIG QUIET)
if (NOT Catch2_FOUND)
	include(FetchContent)
	FetchContent_Declare(Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG v3.8.0
	)
	FetchContent_MakeAvailable(Catch2)
endif()

add_executable(chip8_tests tests/test_chip8.cpp)
target_compile_options(chip8_tests PRIVATE ${CHIP8_WARNINGS})
target_link_libraries(chip8_tests PRIVATE chip8core Catch2::Catch2WithMain)

add_test(NAME chip8_tests COMMAND chip8_tests)

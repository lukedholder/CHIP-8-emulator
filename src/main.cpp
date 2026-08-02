#include "chip8.hpp"
#include "platform.hpp"

#include <SDL.h>
#include <cstdio>
#include <stdexcept>


constexpr int SCALE = 15;
constexpr int CYCLES_PER_FRAME = 10;    // Placeholder

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <rom-file>\n", argv[0]);
        return 1;
    }

    Chip8 chip8;
    if (!chip8.loadROM(argv[1])) {
        std::fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
        return 1;
    }
    
    try {
        Platform platform("Chip-8",
                            Chip8::VIDEO_WIDTH * SCALE,
                            Chip8::VIDEO_HEIGHT * SCALE,
                            Chip8::VIDEO_WIDTH,
                            Chip8::VIDEO_HEIGHT);

        while (platform.processInput()) {
            for (int i = 0; i < CYCLES_PER_FRAME; ++i) {
                chip8.cycle();
            }
            
            platform.update(chip8.videoData());
            SDL_Delay(16);
        }
    }
    catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
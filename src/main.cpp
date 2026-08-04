#include "chip8.hpp"
#include "platform.hpp"

#include <SDL.h>
#include <cstdio>
#include <stdexcept>
#include <chrono>
#include <cstdlib>


constexpr int SCALE = 15;
constexpr int DEFAULT_CYCLES_PER_SECOND = 700;
constexpr double TIMER_INTERVAL = 1.0 / 60.0;
constexpr double MAX_FRAME_TIME = 0.25;


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <rom-file> [cycles-per-second (700 by default)]\n", argv[0]);
        return 1;
    }

    Chip8 chip8;
    if (!chip8.loadROM(argv[1])) {
        std::fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
        return 1;
    }

    int cyclesPerSecond = DEFAULT_CYCLES_PER_SECOND;
    if (argc > 2) {
        const int requested = std::atoi(argv[2]);
        if (requested > 0 && requested < 100000) {
            cyclesPerSecond = requested;
        }
    }
    
    try {
        Platform platform("Chip-8",
                            Chip8::VIDEO_WIDTH * SCALE,
                            Chip8::VIDEO_HEIGHT * SCALE,
                            Chip8::VIDEO_WIDTH,
                            Chip8::VIDEO_HEIGHT);

        using clock = std::chrono::steady_clock;

        auto previous = clock::now();
        double cycleAccumulator = 0.0;
        double timerAccumulator = 0.0;

        while (platform.processInput(chip8.keypadState())) {
            const auto now = clock::now();
            double dt = std::chrono::duration<double>(now - previous).count();
            previous = now;

            if (dt > MAX_FRAME_TIME) {
                dt = MAX_FRAME_TIME;    // don't spriral trying to catch up
            }

            cycleAccumulator += dt * cyclesPerSecond;
            while (cycleAccumulator >= 1.0) {
                chip8.cycle();
                cycleAccumulator -= 1.0;
            }

            bool render = false;
            timerAccumulator += dt;
            while (timerAccumulator >= TIMER_INTERVAL) {
                chip8.tickTimers();
                timerAccumulator -= TIMER_INTERVAL;
                render = true;
            }

            if (render) {
                platform.setBeep(chip8.isBeeping());
                platform.update(chip8.videoData());
            }

            SDL_Delay(1);
        }
    }
    catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
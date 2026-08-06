#include "chip8.hpp"
#include "disasm.hpp"
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


namespace {

uint16_t opcodeAt(const Chip8& chip8, uint16_t addr) {
    return static_cast<uint16_t>((chip8.peek(addr) << 8) | chip8.peek(addr + 1));
}

void dumpState(const Chip8& chip8) {
    const uint16_t pc = chip8.programCounter();
    std::printf("PC=%03X   I=%03X   SP=%u   DT=%u   ST=%u\n", 
        pc, chip8.indexRegister(), chip8.stackPointer(), chip8.delay(), chip8.sound());
    for (int i = 0; i < 16; ++i) {
        std::printf("V%X=%02X ", i, chip8.reg(static_cast<uint8_t>(i)));
    }
    std::printf("\n");
}

}   // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <rom-file> [cycles-per-second] [--schip] [--pause]\n", argv[0]);
        return 1;
    }

    Quirks quirks = Quirks::chip8();
    int cyclesPerSecond = DEFAULT_CYCLES_PER_SECOND;
    bool startPaused = false;

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--schip") {
            quirks = Quirks::superChip();
        }
        else if (arg == "--pause") {
            startPaused = true;
        } else {
            const int requested = std::atoi(arg.c_str());
            if (requested > 0 && requested < 100000) {
                cyclesPerSecond = requested;
            } else {
                std::fprintf(stderr, "Ignoring unrecognized argument: %s\n", arg.c_str());
            }
        }
    }

    Chip8 chip8(quirks);
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

        using clock = std::chrono::steady_clock;

        auto previous = clock::now();
        double cycleAccumulator = 0.0;
        double timerAccumulator = 0.0;

        bool paused = startPaused;

        if (paused) {
            const uint16_t pc = chip8.programCounter();
            const uint16_t opcode = opcodeAt(chip8, pc);
            std::printf("--- paused at entry ---\n%03X: %04X  %s\n", 
                pc, opcode, disassemble(opcode).c_str());
        }

        for (;;) {
            const InputResult input = platform.processInput(chip8.keypadState());
            if (input.quit) {
                break;
            }
            if (input.togglePause) {
                paused = !paused;
                std::printf("--- %s ---\n", paused ? "paused" : "running");
            }
            if (input.dumpState) {
                dumpState(chip8);
            }

            const auto now = clock::now();
            double dt = std::chrono::duration<double>(now - previous).count();
            previous = now;
            if (dt > MAX_FRAME_TIME) {
                dt = MAX_FRAME_TIME;    // don't spriral trying to catch up
            }

            if (paused) {
                cycleAccumulator = 0.0; // don't bank time while stopped
                if (input.step) {
                    const uint16_t pc = chip8.programCounter();
                    std::printf("%03X: %04X  %s\n",
                        pc, opcodeAt(chip8, pc), disassemble(opcodeAt(chip8, pc)).c_str());
                    chip8.cycle();
                }
            } else {
                cycleAccumulator += dt * cyclesPerSecond;
                while (cycleAccumulator >= 1.0) {
                    chip8.cycle();
                    cycleAccumulator -= 1.0;
                }
            }

            bool render = false;
            timerAccumulator += dt;
            while (timerAccumulator >= TIMER_INTERVAL) {
                if (!paused) {
                    chip8.tickTimers();
                }
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
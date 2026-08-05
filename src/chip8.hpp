#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <random>


struct Quirks {
    bool shiftUsesVY = true;            // 8XY6/8XYE read VY instead of VX
    bool loadStoreIncrementsI = true;   // FX55/FX65 advances I by X+1
    bool jumpUsesVX = false;            // BNNN -> BXNN
    bool logicResetsVF = true;          // 8XY1/2/3 zero VF

    static Quirks chip8() { return Quirks{}; }
    static Quirks superChip() { return Quirks{ false, false, true, false}; }
};


class Chip8 {
public:
    static constexpr unsigned int MEMORY_SIZE   = 4096;
    static constexpr unsigned int REGISTER_COUNT = 16;
    static constexpr unsigned int STACK_SIZE    = 16;
    static constexpr unsigned int KEY_COUNT     = 16;
    static constexpr unsigned int VIDEO_WIDTH   = 64;
    static constexpr unsigned int VIDEO_HEIGHT  = 32;
    static constexpr unsigned int FONTSET_SIZE  = 80;

    static constexpr uint16_t START_ADDRESS = 0x200;
    static constexpr uint16_t FONTSET_START_ADDRESS = 0x050;
    
    explicit Chip8(Quirks q = Quirks::chip8());

    uint8_t reg(uint8_t i)      const { return V[i & 0x0F]; }
    uint16_t indexRegister()    const { return I; }
    uint16_t programCounter()   const { return pc; }
    uint8_t stackPointer()      const { return sp; }
    uint16_t stackAt(uint8_t i) const { return stack[i & 0x0F]; }
    uint8_t delay()             const { return delayTimer; }
    uint8_t sound()             const { return soundTimer; }
    uint8_t peek(uint16_t addr) const { return readMemory(addr); }
    bool pixelAt(unsigned int px, unsigned int py) const {
        return video[py * VIDEO_WIDTH + px] != 0;
    }

    bool loadROM(const std::string& filename);
    bool loadROM(const uint8_t* data, std::size_t size);
    void cycle();

    const uint32_t* videoData() const {return video.data(); }

    std::array<uint8_t, KEY_COUNT>& keypadState() { return keypad; }
    
    void tickTimers();

    bool isBeeping() const { return soundTimer > 0; }

private:
    std::array<uint8_t, MEMORY_SIZE>    memory{};
    std::array<uint8_t, REGISTER_COUNT> V{};
    std::array<uint16_t, STACK_SIZE>    stack{};
    std::array<uint8_t, KEY_COUNT>      keypad{};

    std::array<uint32_t, VIDEO_WIDTH * VIDEO_HEIGHT> video{};

    uint16_t I{};   // Index register
    uint16_t pc{};  // Program Counter
    uint8_t sp{};   // Stack Pointer
    uint8_t delayTimer{};
    uint8_t soundTimer{};

    std::array<bool, MEMORY_SIZE> reportedUnknown{};

    Quirks quirks;

    void unknownOpcode(uint16_t opcode);

    uint8_t readMemory(uint16_t addr) const { return memory[addr & 0x0FFF]; }
    void writeMemory(uint16_t addr, uint8_t value) { memory[addr & 0x0FFF] = value; }

    std::mt19937 rng;
    std::uniform_int_distribution<unsigned int> randomByte{0, 255};
};
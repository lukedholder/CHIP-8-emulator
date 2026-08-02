#pragma once

#include <array>
#include <cstdint>
#include <string>

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

    Chip8();

    bool loadROM(const std::string& filename);
    void cycle();

    const uint32_t* videoData() const {return video.data(); }

    void testPattern(); // TEMPORARY

private:
    std::array<uint8_t, MEMORY_SIZE>    memory{};
    std::array<uint8_t, REGISTER_COUNT> V{};
    std::array<uint16_t, STACK_SIZE>    stack{};
    std::array<uint8_t, KEY_COUNT>      keypad{};

    std::array<uint32_t, VIDEO_WIDTH * VIDEO_HEIGHT> video{};

    uint16_t I{};
    uint16_t pc{};
    uint8_t sp{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};

    void unknownOpcode(uint16_t opcode) const;
};
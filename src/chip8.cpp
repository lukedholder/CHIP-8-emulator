#include "chip8.hpp"

#include <fstream>

// Anonymous namespace gives FONTSET internal linkage. 
// It's the modern replacement for 'static' at file scope
namespace {

constexpr std::array<uint8_t, Chip8::FONTSET_SIZE> FONTSET = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

}

Chip8::Chip8() {
    pc = START_ADDRESS;

    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = FONTSET[i];
    }
}

bool Chip8::loadROM(const std::string& filename) {
    // Always open binary files in binary mode to avoid merging the two '\r\n' bytes into one '\n' byte on read (Windows)
    // a ROM is machine code, not text -- any 0x0D 0x0A in it would be corrupted into 0x0A
    std::ifstream file(filename, std::ios::binary | std::ios::ate); // 'std::ios::ate' means "seek to the end on open" so tellg() immediately gives you the file size.
    if (!file) {
        return false;
    }

    const std::streamsize size = file.tellg();
    // Reject oversized ROMs: writing past memory.end() would be a buffer overflow.
    if (size <= 0 || static_cast<unsigned int>(size) > MEMORY_SIZE - START_ADDRESS) {
        return false;
    }

    file.seekg(0, std::ios::beg);

    if (!file.read(reinterpret_cast<char*>(memory.data() + START_ADDRESS), size)) {
        return false;
    }

    return true;
}